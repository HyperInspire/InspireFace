"""Toolkit2 environment smoke test, not an InspireFace accuracy test."""

import tempfile
from pathlib import Path

import numpy as np
import onnx
from onnx import TensorProto, helper, numpy_helper
from rknn.api import RKNN


with tempfile.TemporaryDirectory(prefix="rv1126b-model-") as directory:
    directory = Path(directory)
    graph = helper.make_graph(
        [helper.make_node("Conv", ["image", "weights"], ["output"], kernel_shape=[3, 3])],
        "rv1126b-smoke",
        [helper.make_tensor_value_info("image", TensorProto.FLOAT, [1, 3, 16, 16])],
        [helper.make_tensor_value_info("output", TensorProto.FLOAT, [1, 4, 14, 14])],
        [numpy_helper.from_array(np.full((4, 3, 3, 3), 0.01, dtype=np.float32), "weights")],
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 12)])
    model.ir_version = 8
    onnx.checker.check_model(model)
    onnx.save(model, str(directory / "smoke.onnx"))
    rknn = RKNN()
    try:
        for step, operation in (
            ("config", lambda: rknn.config(target_platform="rv1126b",
                                           mean_values=[[0, 0, 0]], std_values=[[1, 1, 1]])),
            ("load", lambda: rknn.load_onnx(model=str(directory / "smoke.onnx"))),
            ("build", lambda: rknn.build(do_quantization=False)),
            ("export", lambda: rknn.export_rknn(str(directory / "smoke.rknn"))),
        ):
            result = operation()
            if result != 0:
                raise RuntimeError("{} failed: {}".format(step, result))
        if (directory / "smoke.rknn").stat().st_size == 0:
            raise RuntimeError("Empty RKNN model")
        print("RV1126B ONNX -> RKNN conversion passed")
    finally:
        rknn.release()
