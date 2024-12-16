package com.insightface.sdk.inspireface.base;

public class FaceAttributeResult {
    public int num;           ///< Number of faces detected.
    public int[] race;        ///< Race of the detected face.
                              ///< 0: Black;
                              ///< 1: Asian;
                              ///< 2: Latino/Hispanic;
                              ///< 3: Middle Eastern;
                              ///< 4: White;
    public int[] gender;      ///< Gender of the detected face.
                              ///< 0: Female;
                              ///< 1: Male;
    public int[] ageBracket;  ///< Age bracket of the detected face.
                              ///< 0: 0-2 years old;
                              ///< 1: 3-9 years old;
                              ///< 2: 10-19 years old;
                              ///< 3: 20-29 years old;
                              ///< 4: 30-39 years old;
                              ///< 5: 40-49 years old;
                              ///< 6: 50-59 years old;
                              ///< 7: 60-69 years old;
                              ///< 8: more than 70 years old;
}
