from torch import nn
import numpy as np
import torch.optim as optimizer
from matplotlib import pyplot as plt
import cv2
import torch


def reconstruct_image(X, model, store):

    if (store):
        cv2.imwrite('./tmp/origin.png', X[0].cpu().squeeze().numpy() * 255.0)

    # Convert the grayscale image to a float tensor, normalize it, add batch and channel dimensions
    # X_tensor = torch.FloatTensor(X.cpu().numpy()).unsqueeze(0) / 255.0

    # X_tensor = X_tensor.permute(1, 0, 2, 3)
    
    # Disable gradient calculation for inference
    with torch.no_grad():
        # Pass the image through the model
        reconstructed = model(X)
    
    # Convert the tensor to a numpy array and remove the batch dimension
    # Multiply by 255 because the output of the model is normalized between 0 and 1
    # reconstructed_image = reconstructed.cpu().numpy().squeeze() * 255
    
    # # Convert to uint8 to make it compatible with OpenCV image format
    # reconstructed_image = reconstructed_image.astype(np.uint8)
    # print(reconstructed_image.size())

    if (store):
        cv2.imwrite('./tmp/res.png', reconstructed[0].cpu().squeeze().numpy() * 255.0)
    
    return reconstructed


# support for adam, sgd, rmsprop
def get_optimizer(net, optim_type, lr, weight_decay):
    if optim_type == 'adam':
        return optimizer.Adam(net.parameters(), lr=lr, weight_decay=weight_decay)
    elif optim_type == 'sgd':
        return optimizer.SGD(net.parameters(), lr=lr, weight_decay=weight_decay, momentum=0.9)
    elif optim_type == 'rmsprop':
        return optimizer.RMSprop(net.parameters(), lr=lr, weight_decay=weight_decay, momentum=0.9)
    else:
        raise ValueError("Unsupported optimizer type. Choose 'adam', 'sgd', or 'rmsprop'.")