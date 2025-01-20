import numpy as np
import matplotlib.pyplot as plt

def transform_similarity(cosine_sim, threshold=0.48, target_score=0.6, steepness=10):
    """
    Transform cosine similarity to a percentage score between 0-1
    
    Parameters:
        cosine_sim (float): Input cosine similarity value (between -1 and 1)
        threshold (float): Cosine threshold considered as passing grade (default 0.48)
        target_score (float): Target score for threshold value (default 0.6)
        steepness (float): Steepness of the sigmoid curve (default 15)
    
    Returns:
        float: Transformed score between 0-1
    """
    # Map cosine value to sigmoid domain
    x = steepness * (cosine_sim - threshold)
    
    # Apply base sigmoid function
    base_sigmoid = 1 / (1 + np.exp(-x))
    
    # Scale the sigmoid output to start from 0 instead of 0.5
    shifted_sigmoid = 2 * base_sigmoid - 1
    
    # Now shifted_sigmoid ranges from -1 to 1, scale it to 0 to 1
    normalized = (shifted_sigmoid + 1) / 2
    
    # Linear transform to make output equal target_score when cosine_sim = threshold
    threshold_base = 0.5  # the value of normalized when cosine_sim = threshold
    
    # Linear transform parameters
    scale = (1.0 - target_score) / (1.0 - threshold_base)
    
    # Apply linear transform
    final_score = target_score + scale * (normalized - threshold_base)
    
    # Ensure result is between 0-1
    return np.clip(final_score, 0, 1)

if __name__ == "__main__":
    # Generate test values
    values1 = np.arange(-1.0, 0.0, 0.25)  # From -1 to 0, step 0.25
    values2 = np.arange(0.0, 1.01, 0.05)   # From 0 to 1, step 0.05
    test_values = np.concatenate([values1, values2])
    
    # Calculate scores for each value
    scores = [transform_similarity(val) for val in test_values]
    
    # Create plot
    plt.figure(figsize=(10, 6))
    plt.plot(test_values, scores, 'b-', label='Transformation curve')
    
    # Mark threshold point
    threshold = 0.48
    threshold_score = transform_similarity(threshold)
    plt.plot(threshold, threshold_score, 'ro', label=f'Threshold ({threshold}, {threshold_score:.2f})')
    plt.annotate(f'({threshold}, {threshold_score:.2f})',
                xy=(threshold, threshold_score),
                xytext=(threshold-0.2, threshold_score+0.1),
                arrowprops=dict(facecolor='red', shrink=0.05))
    
    # Customize plot
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.xlabel('Cosine Similarity')
    plt.ylabel('Transformed Score')
    plt.title('Cosine Similarity Transformation Curve')
    plt.legend()
    
    # Add reference lines
    plt.axvline(x=threshold, color='r', linestyle='--', alpha=0.3)
    plt.axhline(y=threshold_score, color='r', linestyle='--', alpha=0.3)
    
    plt.show()