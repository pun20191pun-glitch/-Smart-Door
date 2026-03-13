import cv2, dlib, numpy as np, pickle, os, imutils, time
from datetime import datetime
import requests
from threading import Thread
import paho.mqtt.client as mqtt

# ===== MQTT Setup =====
connected_flag = False

def on_connect(client, userdata, flags, rc, properties=None):
    global connected_flag
    print("Connected with result code "+str(rc))
    connected_flag = True
    client.subscribe("arduino/data")

def on_message(client, userdata, message):
    print(f"Received: {message.payload.decode()} on topic {message.topic}")

client = mqtt.Client("PythonClient")
client.on_connect = on_connect
client.on_message = on_message

broker = "10.32.209.223"
port = 1883
client.connect(broker, port)
client.loop_start()

# รอจนกว่าจะเชื่อมต่อ MQTT เสร็จ
while not connected_flag:
    time.sleep(0.1)

# ====== Class สำหรับอ่านกล้องแบบ real-time (ลด delay) ======
class VideoStream:
    def __init__(self, src):
        self.src = src
        self.cap = cv2.VideoCapture(src)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        self.ret, self.frame = self.cap.read()
        self.running = True
        t = Thread(target=self.update, args=())
        t.daemon = True
        t.start()

    def update(self):
        while self.running:
            try:
                if not self.cap.isOpened():
                    print("Reconnect RTSP...")
                    self.cap.release()
                    time.sleep(1)
                    self.cap = cv2.VideoCapture(self.src)
                    self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
                    continue

                ret, frame = self.cap.read()
                if not ret:
                    print("Cannot read frame, retrying...")
                    time.sleep(0.5)
                    continue

                self.ret, self.frame = ret, frame
            except Exception as e:
                print("Camera error:", e)
                time.sleep(1)

    def read(self):
        return self.ret, self.frame

    def stop(self):
        self.running = False
        self.cap.release()

# ====== โหลดโมเดลที่เทรนไว้ ======
model_path = r'D:\DetectFaceApiProd\modeldata'
FACE_DESC, FACE_NAME = pickle.load(open(os.path.join(model_path, 'trained_model.pk'), 'rb'))

# ====== โหลดโมเดลของ dlib ======
detector = dlib.get_frontal_face_detector()
sp = dlib.shape_predictor('shape_predictor_68_face_landmarks.dat')
model = dlib.face_recognition_model_v1('dlib_face_recognition_resnet_model_v1.dat')


# ====== เริ่มเปิดกล้อง RTSP ======
rtsp_url = "rtsp://admin:L2EF7DD6@10.32.209.95/cam/realmonitor?channel=1&subtype=1"
cap = VideoStream(rtsp_url)
lastdetect = ""
last_sent_time = 0  # เวลาในการส่งครั้งล่าสุด

while True:
    try:
        ret, frame = cap.read()
        if not ret:
            print("ไม่สามารถอ่านภาพจากกล้องได้")
            time.sleep(0.5)
            continue

        frame = imutils.resize(frame, width=620)
        img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        dets = detector(img_rgb, 1)

        for d in dets:
            shape = sp(img_rgb, d)
            face_desc0 = model.compute_face_descriptor(img_rgb, shape, 1)

            # คำนวณระยะห่างกับ descriptor ทั้งหมด
            dists = [np.linalg.norm(np.array(fd) - np.array(face_desc0)) for fd in FACE_DESC]
            dists = np.array(dists)
            idx = np.argmin(dists)

            x, y, w, h = d.left(), d.top(), d.width(), d.height()

            if dists[idx] < 0.5:  # threshold
                name = FACE_NAME[idx]
                cv2.putText(frame, name, (x, y-10), cv2.FONT_HERSHEY_COMPLEX, 0.7, (0,255,0), 2)
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0,255,0), 2)

                now = datetime.now()
                current_time = time.time()

                # ===== ส่งทุก 5 วินาที =====
                if current_time - last_sent_time >= 5:
                    print(name + " Come In Gps Time : " + now.strftime("%m/%d/%Y, %H:%M:%S"))

                    # Publish MQTT
                    topic = "arduino/data"
                    command = '{"msg":"BLINK"}'  # คำสั่งเปิด/ปิดแฟลช 5 ครั้ง
                    result = client.publish(topic, command)
                    status = result[0]
                    if status == 0:
                        print(f"Sent BLINK command to {topic}")
                    else:
                        print(f"Failed to send BLINK command to {topic}")


                    last_sent_time = current_time  # อัปเดตเวลา

                lastdetect = name

            else:
                cv2.putText(frame, "UNKNOWN - Contact Admin", (x, y-10), cv2.FONT_HERSHEY_COMPLEX, 0.7, (0,0,255), 2)
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0,0,255), 2)

        cv2.imshow('Face Detect...', frame)
        if cv2.waitKey(1) == 27:  # กด ESC เพื่อออก
            break

    except Exception as e:
        print("An exception occurred:", e)
        time.sleep(1)

cap.stop()
cv2.destroyAllWindows()
