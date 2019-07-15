##//Copyright (c) 2019 aNoŒ¤ ƒvƒŠƒ“‚ğŒ©ç‚é‹Zp
##//https://github.com/anoken/purin_wo_mimamoru_gijutsu/


import io
import numpy as np
import requests
import cv2
import time

from matplotlib import pyplot as plt
time.sleep(2)
res = requests.get('http://192.168.4.1/jpg')//M5Camera IP Adress
bin_data = io.BytesIO(res.content)
file_bytes = np.asarray(bytearray(bin_data.read()), dtype=np.uint8)
img = cv2.imdecode(file_bytes, cv2.IMREAD_COLOR)
cv2.imwrite("m5camera.jpg",img)

