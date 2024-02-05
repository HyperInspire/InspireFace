import os
import sys

# ++ OPTION ++

# Enabling will run all the benchmark tests, which takes time
ENABLE_BENCHMARK_TEST = True

# Testing model name
TEST_MODEL_NAME = "Pikachu-t1"

# ++ END OPTION ++

# Current project path
TEST_PROJECT_PATH = os.path.dirname(os.path.abspath(__file__))

# Current project path
CURRENT_PROJECT_PATH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Main project path
MAIN_PROJECT_PATH = os.path.dirname(CURRENT_PROJECT_PATH)

# Model zip path
MODEL_ZIP_PATH = os.path.join(MAIN_PROJECT_PATH, "test_res/model_zip/")

# Testing model full path
TEST_MODEL_PATH = os.path.join(MODEL_ZIP_PATH, TEST_MODEL_NAME)

# Python test data folder
PYTHON_TEST_DATA_FOLDER = os.path.join(TEST_PROJECT_PATH, "data/")

