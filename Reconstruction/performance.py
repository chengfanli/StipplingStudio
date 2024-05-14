from torch import nn
import numpy as np
import torch.optim as optimizer
from matplotlib import pyplot as plt
import cv2
import torch
import torch.nn.functional as F
from utils import get_optimizer, reconstruct_image

def calculate_loss(reconstructed_img, target_img):
    # Convert the images to PyTorch tensors
    # reconstructed_img_tensor = torch.tensor(reconstructed_img, dtype=torch.float32).unsqueeze(0).unsqueeze(0)
    # target_img_tensor = torch.tensor(target_img, dtype=torch.float32).unsqueeze(0).unsqueeze(0)

    # # Optionally normalize the images to the range [0, 1]
    # reconstructed_img_tensor /= 255.0
    # target_img_tensor /= 255.0

    cv2.imwrite('./tmp/traget.png', target_img[0].cpu().squeeze().numpy() * 255.0)

    # Calculate the MSE loss
    loss = F.mse_loss(reconstructed_img, target_img)

    return loss


def get_performance(model, val_loader, device_str):
    device = torch.device(device_str if device_str == 'cuda' and torch.cuda.is_available() else 'cpu')
    model.to(device)

    loss_all = []
    for _, data in enumerate(val_loader):
        X = data['stippling'].to(device)
        y = data['grayscale'].to(device)

        reconstructed_img = reconstruct_image(X, model, False)

        loss= calculate_loss(reconstructed_img, y)

        loss_all.append(loss.item())

    val_loss = sum(loss_all) / len(loss_all)
    return val_loss
