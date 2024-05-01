import argparse
import math
from Pylette import extract_colors
import configparser
from PIL import Image
import numpy as np
import random
def rgb_to_hex(r, g, b):
    print('#{:02x}{:02x}{:02x}'.format(r, g, b))
    return '#{:02x}{:02x}{:02x}'.format(r, g, b)

def color_distance(c1, c2):
    """ Calculate the Euclidean distance between two RGB colors. """
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(c1, c2)))

def filter_similar_colors(colors, threshold=30):
    """ Remove similar colors based on a distance threshold. """
    filtered_colors = []
    for color in colors:
        if all(color_distance(color.rgb, existing_color.rgb) > threshold for existing_color in filtered_colors):
            filtered_colors.append(color)
    return filtered_colors

def display(colors, w=50, h=50):
    img = Image.new("RGB", size=(w * len(colors), h))
    arr = np.asarray(img).copy()
    # 填充每个颜色
    for i in range(len(colors)):
        c = colors[i]
        # 假设 c.rgb 返回一个 (r, g, b) 元组
        # 将整个列的颜色设置为 c.rgb
        arr[:, i * w : (i + 1) * w, :] = c.rgb

    # 从修改后的 numpy 数组创建图像
    img = Image.fromarray(arr, "RGB")
    img.show()
    
def random_select(items, x):
    if x > len(items):
        return items

    return random.sample(items, x)   

def main():
    config = configparser.ConfigParser()
    # config.read('../inis/test.ini')
    test_path = '../inis/test.ini'
    config.read(test_path)
    source_path = config.get('IO', 'image_path')
    threshold_num = int(config.get('COLOR', 'threshold'))
    x = int(config.get('COLOR', 'size'))
    
    palette = extract_colors(image='../'+source_path, palette_size=10, resize=True)
    '''
    random_colors = palette.random_color(N=8, mode='frequency')
    display(random_colors)
    
    filtered_random_colors = filter_similar_colors(random_colors, threshold=70)
    display(filtered_random_colors)
    
    '''
    filtered_colors = filter_similar_colors(palette, threshold=threshold_num)
    # display(filtered_colors)
    
    filtered_colors = random_select(filtered_colors, x)
    display(filtered_colors)
    
    new_palette = ','.join(rgb_to_hex(color.rgb[0], color.rgb[1], color.rgb[2]) for color in filtered_colors)
    
    config.set('COLOR', 'palette', new_palette)

    with open(test_path, 'w') as configfile:
        config.write(configfile)
    
    
if __name__ == '__main__':
    main()