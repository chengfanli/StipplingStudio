from utils import get_optimizer, reconstruct_image
from performance import calculate_loss, get_performance
import torch
import time
import copy
import cv2
def train_model(model, train_loader, val_loader, num_epoch, parameter, patience, device_str):
    device = torch.device(device_str if device_str == 'cuda' and torch.cuda.is_available() else 'cpu')
    model.to(device)
    model.train()

    loss_all, loss_index = [], []
    num_itration = 0
    best_model, best_val_loss = model, float('inf')
    num_bad_epoch = 0

    optim = get_optimizer(model, parameter["optim_type"], parameter["lr"], parameter["weight_decay"])
    scheduler = torch.optim.lr_scheduler.LinearLR(optim, start_factor=1.0, end_factor=0, total_iters=num_epoch)

    print('------------------------ Start Training ------------------------')
    t_start = time.time()
    loss = 0
    for epoch in range(num_epoch):
        for batch, data in enumerate(train_loader):
            if (batch % 5 == 0):
                print('Batch No. {0}'.format(batch))

            X = data['stippling'].to(device)
            y = data['grayscale'].to(device)

            num_itration += 1

            a = list(model.parameters())[10].clone()

            model.train()
            optim.zero_grad()
            reconstructed_img = model(X)
            if (batch % 10 == 0):
              cv2.imwrite('./tmp/target.png', y[0].cpu().squeeze().numpy() * 255.0)
              cv2.imwrite('./tmp/origin.png', X[0].cpu().squeeze().numpy() * 255.0)
              cv2.imwrite('./tmp/res.png', reconstructed_img[0].cpu().squeeze().detach().numpy() * 255.0)
            loss= calculate_loss(reconstructed_img, y)
            loss.requires_grad_().backward()
            optim.step()

            if (batch % 10 == 0):
              b = list(model.parameters())[10].clone()
              print(torch.equal(a.data, b.data))

            loss_all.append(loss.item())
            loss_index.append(num_itration)

        print('Epoch No. {0} -- loss = {1:.4f}'.format(
            epoch + 1,
            loss.item(),
        ))

        # Validation:
        print('Validation')
        val_loss = get_performance(model, val_loader, device_str)
        model.to(device)
        print("Validation loss: {:.4f}".format(val_loss))

        if val_loss < best_val_loss:
            best_model = copy.deepcopy(model)
            best_val_loss = val_loss
            num_bad_epoch = 0
        else:
            num_bad_epoch += 1

        # early stopping
        if num_bad_epoch >= patience:
            break

        # learning rate scheduler
        scheduler.step()

    t_end = time.time()
    print('Training lasted {0:.2f} minutes'.format((t_end - t_start) / 60))
    print('------------------------ Training Done ------------------------')
    stats = {'loss': loss_all,
             'loss_ind': loss_index,
             }

    return best_model, stats