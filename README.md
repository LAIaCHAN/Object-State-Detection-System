# Object-State-Detection-System
Object State Detection System Based on Cortex-A7   
Operating System: Ubuntu 18.04  
Hardware Platform: Samsung Exynos 4412 Development Board  
Development Tool: arm-linux-gcc

- The project uses a virtual platform bus to separate driver code from device information code.
- The driver layer communicates with the MPU6050 attitude sensor via the I2C bus communication protocol, and the retrieved information is passed to the application layer through the ioctl interface. Additionally, the project uses interrupt number application and input subsystem uevent methods to control the overall system on/off state using independent buttons, with the system's on/off state indicated by LEDs controlled through GPIO.
- The device layer matches and interfaces with the driver layer in various ways, with the most common being device tree matching, followed by ID matching, and finally name matching.
- The application layer utilizes file IO interfaces and ioctl commands to display the retrieved object's three-axis angular velocity information, object acceleration information, and ambient temperature information in the xshell interface.
