import cv2
import numpy as np
import urllib.request
import requests


# ESP32 Camera Stream URL
url = 'http://192.168.29.157/cam-hi.jpg'

# url = 'http://192.168.4.1/cam-hi.jpg'

# YOLO settings
whT = 320
confThreshold = 0.5
nmsThreshold = 0.3

# Load class names
classesfile = 'Python Files/coco.names'
classNames = []
with open(classesfile, 'rt') as f:
    classNames = f.read().rstrip('\n').split('\n')

# Load YOLO model
modelConfig = 'Models/yolov3.cfg'
modelWeights = 'Models/yolov3.weights'
net = cv2.dnn.readNetFromDarknet(modelConfig, modelWeights)
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

# Object detection function

def findObject(outputs, im):
    hT, wT, cT = im.shape
    bbox = []
    classIds = []
    confs = []

    for output in outputs:
        for det in output:
            scores = det[5:]
            classId = np.argmax(scores)
            confidence = scores[classId]
            if confidence > confThreshold:
                w, h = int(det[2]*wT), int(det[3]*hT)
                x, y = int((det[0]*wT) - w/2), int((det[1]*hT) - h/2)
                bbox.append([x, y, w, h])
                classIds.append(classId)
                confs.append(float(confidence))

    indices = cv2.dnn.NMSBoxes(bbox, confs, confThreshold, nmsThreshold)
    print("\nDetected Objects:")
    if len(indices) > 0:
        for i in indices.flatten():
            box = bbox[i]
            x, y, w, h = box
            label = classNames[classIds[i]].upper()
            confidence = int(confs[i] * 100)
            print(f"→ {label} ({confidence}%)")

            # Draw on image
            cv2.rectangle(im, (x, y), (x + w, y + h), (255, 0, 255), 2)
            cv2.putText(im, f'{label} {confidence}%', (x, y - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)

            # Send detected label to ESP32
            try:
                requests.post("http://192.168.29.157/detect", data=label)
            except Exception as e:
                print("Failed to send to ESP32:", e)
    else:
        print("→ No objects detected.")

    hT, wT, cT = im.shape
    bbox = []
    classIds = []
    confs = []

    for output in outputs:
        for det in output:
            scores = det[5:]
            classId = np.argmax(scores)
            confidence = scores[classId]
            if confidence > confThreshold:
                w, h = int(det[2]*wT), int(det[3]*hT)
                x, y = int((det[0]*wT) - w/2), int((det[1]*hT) - h/2)
                bbox.append([x, y, w, h])
                classIds.append(classId)
                confs.append(float(confidence))

    indices = cv2.dnn.NMSBoxes(bbox, confs, confThreshold, nmsThreshold)
    print("\nDetected Objects:")
    if len(indices) > 0:
        for i in indices.flatten():
            box = bbox[i]
            x, y, w, h = box
            label = classNames[classIds[i]].upper()
            confidence = int(confs[i] * 100)
            print(f"→ {label} ({confidence}%)")
            
            # Draw on image
            cv2.rectangle(im, (x, y), (x + w, y + h), (255, 0, 255), 2)
            cv2.putText(im, f'{label} {confidence}%', (x, y - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)
    else:
        print("→ No objects detected.")

# Main loop
while True:
    try:
        img_resp = urllib.request.urlopen(url)
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        im = cv2.imdecode(imgnp, -1)
        
        blob = cv2.dnn.blobFromImage(im, 1/255, (whT, whT), [0, 0, 0], 1, crop=False)
        net.setInput(blob)

        layernames = net.getLayerNames()
        outputNames = [layernames[i - 1] for i in net.getUnconnectedOutLayers().flatten()]
        outputs = net.forward(outputNames)

        findObject(outputs, im)

        cv2.imshow('Image', im)
        if cv2.waitKey(1) == ord('q'):
            break
    except Exception as e:
        print("Error:", e)

cv2.destroyAllWindows()
