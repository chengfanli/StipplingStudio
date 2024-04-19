import torch
from torch.utils.data import Dataset, DataLoader
import cv2
import glob

class DataLoader_Stippling(Dataset):
    def __init__(self, stippling_dir, grayscale_dir):
        """
        Args:
            stippling_dir (string): Directory with all the stippling images.
            grayscale_dir (string): Directory with all the grayscale images.
        """
        self.stippling_files = sorted(glob.glob(stippling_dir + '/*.png'))
        self.grayscale_files = sorted(glob.glob(grayscale_dir + '/*.png'))

    def __len__(self):
        return len(self.stippling_files)

    def __getitem__(self, idx):
        # No need to check if idx is a tensor, since we're not using it in a tensor form
        stippling_img_path = self.stippling_files[idx]
        grayscale_img_path = self.grayscale_files[idx]

        # Load images using cv2
        stippling_img = torch.FloatTensor(cv2.resize(cv2.imread(stippling_img_path, cv2.IMREAD_GRAYSCALE), (800, 592))).unsqueeze(0) / 255.0
        grayscale_img = torch.FloatTensor(cv2.resize(cv2.imread(grayscale_img_path, cv2.IMREAD_GRAYSCALE), (800, 592))).unsqueeze(0) / 255.0

        sample = {'stippling': stippling_img, 'grayscale': grayscale_img}

        return sample

def get_data_loader(stippling_dir, grayscale_dir, batch_size):
    custom_dataset = DataLoader_Stippling(stippling_dir, grayscale_dir)
    return DataLoader(custom_dataset, batch_size=batch_size, shuffle=False, num_workers=8)