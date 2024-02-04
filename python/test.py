from inspireface.modules.core.native import\
HF_ENABLE_FACE_RECOGNITION, HF_ENABLE_LIVENESS, HF_ENABLE_IR_LIVENESS, HF_ENABLE_MASK_DETECT,\
HF_ENABLE_AGE_PREDICT, HF_ENABLE_GENDER_PREDICT, HF_ENABLE_QUALITY, HF_ENABLE_INTERACTION

features = HF_ENABLE_QUALITY | HF_ENABLE_INTERACTION

print(type(features))

if features & HF_ENABLE_QUALITY:
    print("Face quality assessment feature is enabled.")

if features & HF_ENABLE_INTERACTION:
    print("Interaction feature is enabled.")
