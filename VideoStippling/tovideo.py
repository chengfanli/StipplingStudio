import cv2
import os
import numpy as np

def create_video_from_images(image_folder, output_video_file, fps=30):
    # Get all image files in the folder
    images = [img for img in os.listdir(image_folder) if img.endswith(".png")]
    # Sort images by frame number
    images.sort(key=lambda x: int(x.split('_')[1].split('.')[0]))

    # Read the first image to obtain the size
    frame = cv2.imread(os.path.join(image_folder, images[0]))
    height, width, layers = frame.shape

    # Define the codec and create VideoWriter object
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')  # 'mp4v' for .mp4
    video = cv2.VideoWriter(output_video_file, fourcc, fps, (width, height))

    for image in images:
        frame = cv2.imread(os.path.join(image_folder, image))
        if frame is not None:
            video.write(frame)
        else:
            print("Warning: Skipping frame", image, "which could not be read.")

    video.release()
    print("Video created successfully!")

# Usage example
image_folder = './out_frame'  # Folder containing images
output_video_file = './output_video.mp4'  # Output video file
create_video_from_images(image_folder, output_video_file, fps=24)  # Set FPS to desired frames per second