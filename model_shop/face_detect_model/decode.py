import cv2
import numpy as np

def nms(dets, thresh=0.5):
    x1 = dets[:, 0]
    y1 = dets[:, 1]
    x2 = dets[:, 2]
    y2 = dets[:, 3]
    scores = dets[:, 4]

    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        xx1 = np.maximum(x1[i], x1[order[1:]])
        yy1 = np.maximum(y1[i], y1[order[1:]])
        xx2 = np.minimum(x2[i], x2[order[1:]])
        yy2 = np.minimum(y2[i], y2[order[1:]])

        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h
        ovr = inter / (areas[i] + areas[order[1:]] - inter)

        inds = np.where(ovr <= thresh)[0]
        order = order[inds + 1]

    return keep

def distance2bbox(points, distance, max_shape=None):
    """Decode distance prediction to bounding box.

    Args:
        points (Tensor): Shape (n, 2), [x, y].
        distance (Tensor): Distance from the given point to 4
            boundaries (left, top, right, bottom).
        max_shape (tuple): Shape of the image.

    Returns:
        Tensor: Decoded bboxes.
    """
    x1 = points[:, 0] - distance[:, 0]
    y1 = points[:, 1] - distance[:, 1]
    x2 = points[:, 0] + distance[:, 2]
    y2 = points[:, 1] + distance[:, 3]
    if max_shape is not None:
        x1 = x1.clamp(min=0, max=max_shape[1])
        y1 = y1.clamp(min=0, max=max_shape[0])
        x2 = x2.clamp(min=0, max=max_shape[1])
        y2 = y2.clamp(min=0, max=max_shape[0])
    return np.stack([x1, y1, x2, y2], axis=-1)

def distance2kps(points, distance, max_shape=None):
    """Decode distance prediction to bounding box.

    Args:
        points (Tensor): Shape (n, 2), [x, y].
        distance (Tensor): Distance from the given point to 4
            boundaries (left, top, right, bottom).
        max_shape (tuple): Shape of the image.

    Returns:
        Tensor: Decoded bboxes.
    """
    preds = []
    for i in range(0, distance.shape[1], 2):
        px = points[:, i % 2] + distance[:, i]
        py = points[:, i % 2 + 1] + distance[:, i + 1]
        if max_shape is not None:
            px = px.clamp(min=0, max=max_shape[1])
            py = py.clamp(min=0, max=max_shape[0])
        preds.append(px)
        preds.append(py)
    return np.stack(preds, axis=-1)

def decoding(data, input_size=(320, 320)):
    scores_list = []
    bboxes_list = []
    kpss_list = []
    input_height, input_width = input_size
    _feat_stride_fpn = [8, 16, 32]
    fmc = 3
    _num_anchors = 2
    center_cache = {}
    thresh = 0.4
    max_num = 0
    for idx, stride in enumerate(_feat_stride_fpn):
        scores = data[idx]
        bbox_preds = data[idx + fmc]
        bbox_preds = bbox_preds * stride
        kps_preds = data[idx + fmc * 2] * stride
        height = input_height // stride
        width = input_width // stride
        key = (height, width, stride)
        if key in center_cache:
            anchor_centers = center_cache[key]
        else:
            anchor_centers = np.stack(np.mgrid[:height, :width][::-1], axis=-1).astype(np.float32)
            anchor_centers = (anchor_centers * stride).reshape((-1, 2))
            if _num_anchors > 1:
                anchor_centers = np.stack([anchor_centers] * _num_anchors, axis=1).reshape((-1, 2))
            if len(center_cache) < 100:
                center_cache[key] = anchor_centers

        pos_inds = np.where(scores >= thresh)[0]
        bboxes = distance2bbox(anchor_centers, bbox_preds)
        pos_scores = scores[pos_inds]
        pos_bboxes = bboxes[pos_inds]
        scores_list.append(pos_scores)
        bboxes_list.append(pos_bboxes)
        pos_scores = scores[pos_inds]
        pos_bboxes = bboxes[pos_inds]
        scores_list.append(pos_scores)
        bboxes_list.append(pos_bboxes)
        kpss = distance2kps(anchor_centers, kps_preds)
        # kpss = kps_preds
        kpss = kpss.reshape((kpss.shape[0], -1, 2))
        pos_kpss = kpss[pos_inds]
        kpss_list.append(pos_kpss)

    scores = np.vstack(scores_list)
    scores_ravel = scores.ravel()
    order = scores_ravel.argsort()[::-1]
    bboxes = np.vstack(bboxes_list)
    kpss = np.vstack(kpss_list)
    pre_det = np.hstack((bboxes, scores)).astype(np.float32, copy=False)
    pre_det = pre_det[order, :]
    keep = nms(pre_det)
    det = pre_det[keep, :]
    # kpss = kpss[order, :, :]
    # kpss = kpss[keep, :, :]
    print(det)
    return det


if __name__ == '__main__':
    loaded_data_list = np.load('pickle.npy', allow_pickle=True)
    # for arr in loaded_data_list:
    #     print(arr.shape)
    img = cv2.imread("input.jpg")
    dets = decoding(loaded_data_list)

    for det in dets:
        x1, y1, x2, y2 = np.asarray(det[:4]).astype(int)
        cv2.rectangle(img, (x1, y1), (x2, y2), (255, 0, 233), 1)

    cv2.imshow("w", img)
    cv2.waitKey(0)

    print(loaded_data_list[2].shape)
    print(np.squeeze(loaded_data_list[2]))