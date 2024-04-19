from torch import nn
import numpy as np
import torch.optim as optimizer
from matplotlib import pyplot as plt
import cv2
import torch
import torch.nn.functional as F
from utils import get_optimizer, reconstruct_image

def calculate_loss(reconstructed_img, target_img):
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