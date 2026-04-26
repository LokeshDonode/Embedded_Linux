# Buildroot Custom Linux Environment (Dockerized)

This guide provides a simple, step-by-step process for building a custom Linux system using Buildroot inside a Docker container. Using Docker ensures that your host machine stays clean and that the build environment is consistent for everyone.

## 📋 Prerequisites
Before you begin, ensure you have **Docker** installed and running on your host machine.

---

## 🚀 Step-by-Step Guide

### Step 1: Project Setup
First, create a dedicated folder for your project and navigate into it.

```bash
mkdir buildroot-project
cd buildroot-project
```

> **Note:** Before moving to the next step, make sure you have placed your `Dockerfile`, `quick_build.sh`, and `test_system.sh` files from the repository into this new `buildroot-project` folder.

### Step 2: Build the Docker Image
Next, we will build the Docker image. This creates the isolated environment equipped with all the tools necessary to run Buildroot.

```bash
docker build -t buildroot-env .
```

### Step 3: Run the Container and Compile
Now, start the container interactively. This will drop you into the container's command line.

```bash
docker run -it --name buildroot-container buildroot-env
```

**Inside the container**, run the following commands to make your scripts executable and start the build process:

```bash
# Make the scripts executable
chmod +x quick_build.sh test_system.sh

# Start compiling the Buildroot system
./quick_build.sh

# (Optional) Test the newly built system inside the container
./test_system.sh
```

### Step 4: Extract the Built System
Once the build is completely finished, you can exit the container by typing `exit`.

Back on your **host machine**, copy the built files out of the container and onto your local drive.

```bash
# Extract the files to a new folder called 'built-system'
docker cp buildroot-container:/buildroot-work/buildroot/output/images ./built-system

# Verify the files were copied successfully
ls -la built-system/
```

---

## 📦 Expected Output
If the build was successful, you will see the following files inside your `built-system` folder:

* **`bzImage`** - The compiled Linux kernel (~5-10 MB)
* **`rootfs.cpio`** - The root filesystem archive (~5-10 MB)
* **`rootfs.tar`** - A tarball version of the filesystem

---

## 🧹 Clean-up
Once you have safely extracted your files, you can remove the Docker container and image to free up disk space.

```bash
# Stop and remove the container
docker stop buildroot-container
docker rm buildroot-container

# (Optional) Remove the Docker image
docker rmi buildroot-env
```