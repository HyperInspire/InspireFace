import numpy as np

def postprocess(feat):
    feat_normal = feat / np.sqrt(np.dot(feat, feat.T))
    return feat_normal

mat = np.load("f.npy")
n1 = postprocess(mat[0, 0, 0, ])
n2 = postprocess(mat[1, 0, 0, ])
n3 = postprocess(mat[2, 0, 0, ])

print(np.dot(n1, n2.T))
print(np.dot(n3, n2.T))
print(np.dot(n1, n3.T))

print(mat[0, 0, 0])