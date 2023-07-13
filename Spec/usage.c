ret_t usage_async_write()
{
    ret_t                 res          = RET_OK;
    u32                   data_size    = 1024;
    u32                   timeout_ms   = 1000;
    async_file_accessor_t fileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
    async_file_access_request_t newRequest = 
    {
        .info = 
        {
            .direction  = ASYNC_FILE_ACCESS_WRITE;
            .fn         = "./test_aio_write_1.txt";
            .size       = data_size;
            .offset     = 0;
        }
    };

    res = fileAccessor.getRequest(&fileAccessor, &newRequest);

    if(RET_OK == res)
    {
        void *buf   = fileAccessor.getBuf(&fileAccessor, &newRequest);

        memset(buf, 'a', data_size);
        res = fileAccessor.putRequest(&fileAccessor, &newRequest);
    }

    if(RET_OK == res)
    {
        res = fileAccessor.waitRequest(&fileAccessor, &newRequest, timeout_ms);

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s", newRequest.info.fn);
            res = fileAccessor.cancelRequest(&fileAccessor, &newRequest);
        }
    }

    res = fileAccessor.cancelAll(&fileAccessor);

    return res;
};