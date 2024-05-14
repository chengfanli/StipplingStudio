import cv2
import os

def save_last_10_seconds_frames(video_path, output_dir):
    # Create the output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Capture the video from the file
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return

    # Get the total number of frames and the frames per second in the video
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    
    # Calculate the starting frame of the last 10 seconds
    start_frame = max(0, total_frames - int(10 * fps))
    
    # Set video position to the starting frame of the last 10 seconds
    # cap.set(cv2.CAP_PROP_POS_FRAMES, start_frame)
    
    frame_count = 0
    
    while True:
        # Read a new frame
        ret, frame = cap.read()
        if not ret:
            break  # Break the loop if there are no frames to read
        
        # Save the frame
        cv2.imwrite(f'{output_dir}/frame_{frame_count:04d}.jpg', frame)
        frame_count += 1

    cap.release()
    print(f'Saved {frame_count} frames from the last 10 seconds to {output_dir}')

# Usage example
video_path = './stippling.mp4'
output_dir = './frame'
save_last_10_seconds_frames(video_path, output_dir)