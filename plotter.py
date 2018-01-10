import matplotlib.pyplot as plt
import os

# Detector pixels: find data files
location = os.getcwd()
txtfiles = []

for file in os.listdir(location):
    try:
        if file.endswith(".txt") and file.startswith("detector"):
            txtfiles.append(str(file))

    except Exception as e:
        raise e
print(txtfiles)

# Initialise plot
fig, ax = plt.subplots(nrows=2, ncols=1)

# Get data
for filename in txtfiles:
    file = open(filename, 'r')
    data = {}
    dim = 0
    for line in file:
        data[dim] = line.split()
        dim += 1
    # Add to plot
    ax[0].scatter(data[0], data[1], s=0.75, marker='x')  # x and y
    ax[1].scatter(data[0], data[2], s=0.75, marker='x')  # x and z

ax[0].set_title('XY-plane pixel locations')
ax[1].set_title('XZ-plane pixel locations')
plt.axis('equal')
plt.show()
