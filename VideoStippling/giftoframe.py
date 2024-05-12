from PIL import Image

def extract_frames(gif_path, output_folder):
    with Image.open(gif_path) as img:
        for frame in range(img.n_frames):
            img.seek(frame)
            img.save(f"{output_folder}/frame_{frame}.png")

# Replace 'path_to_gif.gif' with the path to your GIF file
# Replace 'output_folder' with the path where you want to save the frames
extract_frames('./intro.gif', './frame')