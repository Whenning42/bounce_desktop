OUTDIR="${PLATFORM_INFO_OUTDIR:-}"

eglinfo -B | grep "platform\|renderer\|Device #" > ${OUTDIR}/eglinfo.txt
glxinfo -B > ${OUTDIR}/glxinfo.txt
vulkaninfo --summary > ${OUTDIR}/vkinfo.txt
