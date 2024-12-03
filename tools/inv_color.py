import cv2

img = cv2.imread("/Users/tunm/Downloads/企业微信截图_2da9e222-2c7d-4cfc-9be0-d5dd0a261a2d.png")
print(img.shape)

gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
cv2.imshow("gray", gray)
cv2.waitKey(0)
