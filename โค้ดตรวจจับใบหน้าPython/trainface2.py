import cv2
import numpy as np
import pickle
import dlib
import os

# ตั้งค่า path
path = './facedata/'
model_path = r'D:\DetectFaceApiProd\modeldata'

# โหลดโมเดลของ dlib
detector = dlib.get_frontal_face_detector()
sp = dlib.shape_predictor('shape_predictor_68_face_landmarks.dat')
model = dlib.face_recognition_model_v1('dlib_face_recognition_resnet_model_v1.dat')

FACE_DESC = []
FACE_NAME = []

# loop อ่านรูปจากโฟลเดอร์
for fn in os.listdir(path):
    if fn.lower().endswith('.jpg'):
        file_path = os.path.join(path, fn)
        img = cv2.imread(file_path)

        if img is None:
            print(f"ไม่สามารถอ่านรูปได้: {file_path}")
            continue

        # แปลง BGR -> RGB
        img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

        # ตรวจจับใบหน้า
        dets = detector(img_rgb, 1)

        if len(dets) == 0:
            print(f"ไม่เจอใบหน้าใน {fn}")
            continue

        for k, d in enumerate(dets):
            shape = sp(img_rgb, d)
            face_descriptor = model.compute_face_descriptor(img_rgb, shape, 1)

            FACE_DESC.append(np.array(face_descriptor))
            FACE_NAME.append(fn[:fn.index('_')])

        print('picture loading...', fn)

# เซฟโมเดล
save_path = os.path.join(model_path, 'trained_model.pk')
with open(save_path, 'wb') as f:
    pickle.dump((FACE_DESC, FACE_NAME), f)

print('train complete....')