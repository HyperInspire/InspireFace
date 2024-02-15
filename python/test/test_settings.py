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

# Stores some temporary file data generated during testing
TMP_FOLDER = os.path.join(CURRENT_PROJECT_PATH, "tmp")

# Default db file path
DEFAULT_DB_PATH = os.path.join(TMP_FOLDER, ".E63520A95DD5B3892C56DA38C3B28E551D8173FD")

# Create tmp if not exist
os.makedirs(TMP_FOLDER, exist_ok=True)