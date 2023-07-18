
//Convert YUV to Analog RGB BMP file and dump it, default dumper
void DumpBmpA(char *pFname, IMG_TYPE yuvType, u16 shtBits, int mirrorV) //yuvType YUV444/YUV422/YUV420;
{
    char pFn[FILE_NAME_SIZE] = { 0 };
    int yData;
    int uData;
    int vData;
    int byteAlign; //BMP width should be divided by 4
    int x, row, col, val;

    if(sParams.sParaGeneral.dump_level != 1 || (SANITY_DUMP == 0 && sParams.sParaGeneral.sanity_dump_all == 0))
    {
        return;
    }

    if (sParams.sParaInOutFile.output_infile_name == 0)
        sprintf(pFn, "%s%03d_%02d_%s_F%04d.bmp", DUMP_DIR, pIspFlowPara->curLoop, pIspFlowPara->pipePos, pFname, pIspFlowPara->curFrame);
    else
        sprintf(pFn, "%s%03d_%02d_%s_F%04d.bmp", DUMP_DIR, pIspFlowPara->curLoop, pIspFlowPara->pipePos, sParams.sParaInOutFile.output_file_name, pIspFlowPara->curFrame);


    byteAlign = (pImageIsp->imageWidth % 4);

    int fd = open(pFn,  O_RDWR |O_CREAT |O_TRUNC , 00777);
    if(fd < 0)
    {
        close(fd);
        fd = open(pFn,  O_RDWR |O_CREAT |O_TRUNC , 00777);
        if(fd < 0)
        {
            close(fd);
            perror(" open");
            DbgPrint("Info", " DUMP Fail\n");
            return;
        }
    }
    int length = pImageIsp->imageHeight * (pImageIsp->imageWidth + byteAlign) * 3 + 54;
    ftruncate(fd, length);
    //MMAP dump bmp
    char *buffer = (char *)mmap(NULL, length, PROT_WRITE, MAP_SHARED, fd, 0);
    if(buffer == MAP_FAILED)
    {
         char *buffer = (char *)mmap(NULL, length, PROT_WRITE, MAP_SHARED, fd, 0);
         if(buffer == MAP_FAILED)
         {
             perror("  mmap");
             DbgPrint("Info", " DUMP Fail\n");
             return;
         }
    }
    BmpHeaderChar(buffer, RGB, mirrorV);
    int index = 0;


    for (row = 0; row < pImageIsp->imageHeight; row++)
    {
        for (col = 0; col < pImageIsp->imageWidth; col++)
        {
            yData = GetValue16(pImageIsp->pYUV[ISP_Y], row, col);
            if (yuvType == YUV444)
            {
                uData = GetValue16(pImageIsp->pYUV[ISP_U], (int)(row / rowSep), (int)(col / colSep));
                vData = GetValue16(pImageIsp->pYUV[ISP_V], (int)(row / rowSep), (int)(col / colSep));
            }
            else
            {
                uData = GetValueUV16(pImageIsp->pYUV[ISP_U], (int)(row / rowSep), (int)(col / colSep));
                vData = GetValueUV16(pImageIsp->pYUV[ISP_V], (int)(row / rowSep), (int)(col / colSep));
            }
            yData = yData >> shtBits;
            uData = uData >> shtBits;
            vData = vData >> shtBits;
            val = YUV2RGB_AnaHdB(yData, uData, vData, 8);
            val = Clip(val, 0, 255);
            buffer[index + 54] = val;
            index++;
            val = YUV2RGB_AnaHdG(yData, uData, vData, 8);
            val = Clip(val, 0, 255);
            buffer[index + 54] = val;
            index++;
            val = YUV2RGB_AnaHdR(yData, uData, vData, 8);
            val = Clip(val, 0, 255);
            buffer[index + 54] = val;
            index++;
            if(col == (pImageIsp->imageWidth - 1))
            {
                val = 0;
                for (x = 0; x < byteAlign; x++)
                {
                    buffer[index + 54] = val;
                    index++;
                }
            }
        }
    }

    if(msync(buffer, length ,MS_ASYNC) < 0)
    {
        perror(" msync");
        DbgPrint("Info", " DUMP Fail\n");
        return;
    }
    munmap(buffer, length);
    if(close(fd) == -1)
    {
        perror(" close");
        DbgPrint("Info", " DUMP Fail\n");
        return;
    }
    DbgPrint("Info", " DUMP %s\n", pFn);
}