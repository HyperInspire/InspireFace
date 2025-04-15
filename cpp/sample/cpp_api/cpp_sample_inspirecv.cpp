#include <iostream>
#include <inspirecv/inspirecv.h>

int main() {
    /* Image I/O */

    // Load image from file
    // Load with 3 channels (RGB)
    inspirecv::Image img = inspirecv::Image::Create("test_res/data/bulk/kun_cartoon_crop.jpg", 3);  

    // Load image from buffer
    // uint8_t* buffer = ...;  // buffer is a pointer to the image data
    // bool is_alloc_mem = false;  // if true, will allocate memory for the image data, 
    //                             // false is recommended to point to the original data to avoid copying
    // inspirecv::Image img_buffer = inspirecv::Image::Create(width, height, channel, buffer, is_alloc_mem);

    // Save image to file
    img.Write("cv/output.jpg");

    // Show image, warning: it must depend on opencv
    // img.Show("input");

    // Get pointer to image data
    const uint8_t* ptr = img.Data();

    /* Image Processing */
    // Convert to grayscale
    inspirecv::Image gray = img.ToGray();
    gray.Write("cv/gray.jpg");

    // Apply Gaussian blur
    inspirecv::Image blurred = img.GaussianBlur(3, 1.0);
    blurred.Write("cv/blurred.jpg");
    
    // Geometric transformations
    auto scale = 0.35;
    inspirecv::Image resized = img.Resize(img.Width() * scale, img.Height() * scale);  // Resize image
    resized.Write("cv/resized.jpg");

    // Rotate 90 degrees clockwise
    inspirecv::Image rotated = img.Rotate90();
    rotated.Write("cv/rotated.jpg");

    // Flip vertically
    inspirecv::Image flipped_vertical = img.FlipVertical();
    flipped_vertical.Write("cv/flipped_vertical.jpg");

    // Flip horizontally
    inspirecv::Image flipped_horizontal = img.FlipHorizontal();
    flipped_horizontal.Write("cv/flipped_horizontal.jpg");

    // Crop for rectangle
    inspirecv::Rect<int> rect = inspirecv::Rect<int>::Create(78, 41, 171, 171);
    inspirecv::Image cropped = img.Crop(rect);
    cropped.Write("cv/cropped.jpg");

    // Image padding
    int top = 50, bottom = 50, left = 50, right = 50;
    inspirecv::Image padded = img.Pad(top, bottom, left, right, inspirecv::Color::Green);
    padded.Write("cv/padded.jpg");

    // Swap red and blue channels
    inspirecv::Image swapped = img.SwapRB();
    swapped.Write("cv/swapped.jpg");

    // Multiply image by scale factor
    double scale_factor = 0.5;
    inspirecv::Image scaled = img.Mul(scale_factor);
    scaled.Write("cv/scaled.jpg");

    // Add value to image
    double value = 20;
    inspirecv::Image added = img.Add(value);
    added.Write("cv/added.jpg");

    // Rotate 90 degrees clockwise(also support 270 and 180)
    inspirecv::Image rotated_90 = img.Rotate90();
    rotated_90.Write("cv/rotated_90.jpg");

    // Affine transform
    /**
     * Create a transform matrix from the following matrix
     * [[a11, a12, tx],
     *  [a21, a22, ty]]
     * 
     * Face crop transform matrix
     * [[0.0, -1.37626, 261.127],
     *  [1.37626, 0.0, 85.1831]]
     */
    float a11 = 0.0f;
    float a12 = -1.37626f;
    float a21 = 1.37626f;
    float a22 = 0.0f;
    float b1 = 261.127f;
    float b2 = 85.1831f;

    inspirecv::TransformMatrix trans = inspirecv::TransformMatrix::Create(a11, a12, b1, 
                                                                          a21, a22, b2 );
    int dst_width = 112;
    int dst_height = 112;
    inspirecv::Image affine = rotated_90.WarpAffine(trans, dst_width, dst_height);
    affine.Write("cv/affine.jpg");
    
    return 0;
}
