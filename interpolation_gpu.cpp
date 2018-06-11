#include "interpolation_gpu.h"

void init(size_t groupSizeX, size_t groupSizeY, size_t& ResImageH, size_t& ResImageW, cl_context& context, cl_command_queue& commandQueue, cl_program& program, char* filename, Image& image, cl_mem& inputImageBuffer, cl_mem& widthBuffer, cl_mem& heightBuffer, cl_mem& angleBuffer, cl_mem& depthBuffer, cl_mem& CurProbeRadiusBuffer, cl_mem& outputImageBuffer)
{
    cl_int status = 0;
    cl_device_type dType = CL_DEVICE_TYPE_GPU;
    cl_platform_id *platforms = NULL;
    cl_device_id   device;
    cl_uint     num_platforms;


    //Setup the OpenCL Platform,
    //Get the first available platform. Use it as the default platform
    status = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
    status = clGetPlatformIDs(num_platforms, platforms, NULL);
    LOG_OCL_ERROR(status, "clGetPlatformIDs Failed.");

    //Get the first available device
    status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    LOG_OCL_ERROR(status, "clGetDeviceIDs Failed.");

    //Query Platform Information
    char queryBuffer[1024];
    status = clGetPlatformInfo(platforms[0], CL_PLATFORM_NAME, 1024, &queryBuffer, NULL);
    LOG_OCL_ERROR(status, "clGetPlatformInfo Failed.");
    printf("clGetPlatformInfo: %s\n", queryBuffer);

    //Query Device Information
    cl_device_type deviceType;
    char deviceName[1024], deviceVendor[1024];
    status = clGetDeviceInfo(device, CL_DEVICE_VENDOR, 1024, &deviceVendor, NULL);
    printf("CL_DEVICE_VENDOR: %s\n", deviceVendor);
    status = clGetDeviceInfo(device, CL_DEVICE_NAME, 1024, &deviceName, NULL);
    printf("CL_DEVICE_NAME: %s\n", deviceName);
    status = clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, NULL);
    printf("CL_DEVICE_TYPE: %d\n", deviceType);

    cl_context_properties cps[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms[0],
        0
    };
    context = clCreateContextFromType(
        cps,
        dType,
        NULL,
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateContextFromType Failed.");

//    return context;

    // Create command queue
    commandQueue = clCreateCommandQueue(context,
        device,
        CL_QUEUE_PROFILING_ENABLE,
        &status);
    LOG_OCL_ERROR(status, "clCreateCommandQueue Failed.");


    int Lnum = image.width;
    int Snum = image.height;
    float Angle = image.angle;
    float Depth = image.depth;
    float CurProbeRadius = image.radius;
    int iBHeightResolve = 1024;
    float	ProbeRadiusPixel = CurProbeRadius * Snum / Depth;
    float SectorRadiusPixel = ProbeRadiusPixel + Snum;
    ResImageH = 1024;
    float Ratio = ResImageH / (SectorRadiusPixel - cos(Angle)*ProbeRadiusPixel + 1);
    ResImageW = round(sin(Angle)*SectorRadiusPixel * 2 * Ratio + 1);

    //Create OpenCL device input buffer
    inputImageBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(float) * image.width * image.height,
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the image buffer.");
    widthBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(int),
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the imgPara1 buffer.");
    heightBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(int),
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the imgPara1 buffer.");
    angleBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(float),
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the imgPara2 buffer.");
    depthBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(float),
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the imgPara2 buffer.");
    CurProbeRadiusBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(float),
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the imgPara2 buffer.");

    outputImageBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(float) * ResImageH * ResImageW,
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateBuffer Failed while creating the image buffer.");

    //Set input data
    cl_event writeEvt[6];
    status = clEnqueueWriteBuffer(commandQueue,
        inputImageBuffer,
        CL_TRUE,
        0,
        image.width * image.height * sizeof(float),
        image.data,
        0,
        NULL,
        &writeEvt[0]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the image data.");
    status = clEnqueueWriteBuffer(commandQueue,
        widthBuffer,
        CL_TRUE,
        0,
        sizeof(int),
        &image.width,
        0,
        NULL,
        &writeEvt[1]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the widthBuffer.");
    status = clEnqueueWriteBuffer(commandQueue,
        heightBuffer,
        CL_TRUE,
        0,
        sizeof(int),
        &image.height,
        0,
        NULL,
        &writeEvt[2]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the heightBuffer.");
    status = clEnqueueWriteBuffer(commandQueue,
        angleBuffer,
        CL_TRUE,
        0,
        sizeof(float),
        &image.angle,
        0,
        NULL,
        &writeEvt[3]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the angleBuffer.");
    status = clEnqueueWriteBuffer(commandQueue,
        depthBuffer,
        CL_TRUE,
        0,
        sizeof(float),
        &image.depth,
        0,
        NULL,
        &writeEvt[4]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the depthBuffer.");
    status = clEnqueueWriteBuffer(commandQueue,
        CurProbeRadiusBuffer,
        CL_TRUE,
        0,
        sizeof(float),
        &image.radius,
        0,
        NULL,
        &writeEvt[5]);
    LOG_OCL_ERROR(status, "clEnqueueWriteBuffer Failed while writing the CurProbeRadiusBuffer.");

    status = clFinish(commandQueue);
    LOG_OCL_ERROR(status, "clFinish Failed while writing the image data and parameters.");


    ifstream kernelFile(filename, ios::in);
    if (!kernelFile.is_open())
    {
        cerr << "Failed to open file for reading: " << filename << endl;
    }
    ostringstream oss;
    oss << kernelFile.rdbuf();
    string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();

    // Create a program from the kernel source
    program = clCreateProgramWithSource(context, 1,
        (const char **)&srcStr, NULL, &status);
    LOG_OCL_ERROR(status, "clCreateProgramWithSource Failed.");

    // Build the program
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != CL_SUCCESS)
    {
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
        printf("%s", buildLog);
        clReleaseProgram(program);
    }

}

cl_kernel createKernel(cl_program& program, char* kernel_name, cl_mem& inputImageBuffer, cl_mem& widthBuffer, cl_mem& heightBuffer, cl_mem& angleBuffer, cl_mem& depthBuffer, cl_mem& CurProbeRadiusBuffer)
{
    cl_int status = 0;
    cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
    LOG_OCL_ERROR(status, "clCreateKernel interpolation_kernel Failed.");

    // Set the arguments of the kernel
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&inputImageBuffer);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&widthBuffer);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&heightBuffer);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&angleBuffer);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&depthBuffer);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&CurProbeRadiusBuffer);
    LOG_OCL_ERROR(status, "clSetKernelArg Failed.");

    return kernel;

}


