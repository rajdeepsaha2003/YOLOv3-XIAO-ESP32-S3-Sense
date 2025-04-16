import cv2
import urllib.request
import numpy as np

# Replace the URL with the IP camera's stream URL
url = 'http://192.168.29.157/cam-hi.jpg'

# Optional: Create a resizable window
cv2.namedWindow("live Cam Testing", cv2.WINDOW_AUTOSIZE)

# Create a VideoCapture object (you don't need this if using urllib for frames)
# cap = cv2.VideoCapture(url)  # Can be removed

# Read and display video frames
while True:
    try:
        # Read image bytes from the stream
        img_resp = urllib.request.urlopen(url)
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        im = cv2.imdecode(imgnp, -1)

        # Show the frame
        cv2.imshow('live Cam Testing', im)

        # Press 'q' to quit
        if cv2.waitKey(5) & 0xFF == ord('q'):
            break

    except Exception as e:
        print(f"Error: {e}")
        break

cv2.destroyAllWindows()
