eglinfo -B | grep "platform\|renderer\|Device #" > eglinfo.txt
glxinfo -B > glxinfo.txt
vulkaninfo --summary > vkinfo.txt
