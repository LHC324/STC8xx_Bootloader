

static rt_err_t _rym_do_recv(
    struct rym_ctx *ctx,
    int handshake_timeout)
{
    rt_err_t err;

    ctx->stage = RYM_STAGE_NONE;

    ctx->buf = rt_malloc(_RYM_STX_PKG_SZ);
    if (ctx->buf == RT_NULL)
        return -RT_ENOMEM;

    err = _rym_do_handshake(ctx, handshake_timeout);
    if (err != RT_EOK)
    {
        rt_free(ctx->buf);
        return err;
    }
    while (1)
    {
        err = _rym_do_trans(ctx);
        if (err != RT_EOK)
        {
            rt_free(ctx->buf);
            return err;
        }

        err = _rym_do_fin(ctx);
        if (err != RT_EOK)
        {
            rt_free(ctx->buf);
            return err;
        }

        if (ctx->stage == RYM_STAGE_FINISHED)
            break;
    }
    rt_free(ctx->buf);
    return err;
}


static rt_err_t _rym_do_handshake(
    struct rym_ctx *ctx,
    int tm_sec)
{
    enum rym_code code;
    rt_size_t i;
    rt_uint16_t recv_crc, cal_crc;
    rt_size_t data_sz;
    rt_tick_t tick;

    ctx->stage = RYM_STAGE_ESTABLISHING;
    /* send C every second, so the sender could know we are waiting for it. */
    for (i = 0; i < tm_sec; i++)
    {
        _rym_putchar(ctx, RYM_CODE_C);
        code = _rym_read_code(ctx,
                              RYM_CHD_INTV_TICK);
        if (code == RYM_CODE_SOH)
        {
            data_sz = _RYM_SOH_PKG_SZ;
            break;
        }
        else if (code == RYM_CODE_STX)
        {
            data_sz = _RYM_STX_PKG_SZ;
            break;
        }
    }
    if (i == tm_sec)
    {
        return -RYM_ERR_TMO;
    }

    /* receive all data */
    i = 0;
    /* automatic exit after receiving specified length data, timeout: 100ms */
    tick = rt_tick_get();
    while (rt_tick_get() <= (tick + rt_tick_from_millisecond(100)) && i < (data_sz - 1))
    {
        i += _rym_read_data(ctx, data_sz - 1);
        rt_thread_mdelay(5);
    }

    if (i != (data_sz - 1))
        return -RYM_ERR_DSZ;

    /* sanity check */
    if (ctx->buf[1] != 0 || ctx->buf[2] != 0xFF)
        return -RYM_ERR_SEQ;

    recv_crc = (rt_uint16_t)(*(ctx->buf + data_sz - 2) << 8) | *(ctx->buf + data_sz - 1);
    cal_crc = CRC16(ctx->buf + 3, data_sz - 5);
    if (recv_crc != cal_crc)
        return -RYM_ERR_CRC;

    /* congratulations, check passed. */
    if (ctx->on_begin && ctx->on_begin(ctx, ctx->buf + 3, data_sz - 5) != RYM_CODE_ACK)
        return -RYM_ERR_CAN;

    return RT_EOK;
}

static rt_err_t _rym_do_trans(struct rym_ctx *ctx)
{
    _rym_putchar(ctx, RYM_CODE_ACK);
    _rym_putchar(ctx, RYM_CODE_C);
    ctx->stage = RYM_STAGE_ESTABLISHED;

    while (1)
    {
        rt_err_t err;
        enum rym_code code;
        rt_size_t data_sz, i;
        rt_size_t errors = 0;

        code = _rym_read_code(ctx,
                              RYM_WAIT_PKG_TICK);
        switch (code)
        {
        case RYM_CODE_SOH:
            data_sz = 128;
            break;
        case RYM_CODE_STX:
            data_sz = 1024;
            break;
        case RYM_CODE_EOT:
            return RT_EOK;
        default:
            // return -RYM_ERR_CODE;
            goto __ERROR_HANDLE;
        };

        err = _rym_trans_data(ctx, data_sz, &code);
        if (err != RT_EOK)
        {
__ERROR_HANDLE:  
            /* the spec require multiple CAN */
            for (i = 0; i < RYM_END_SESSION_SEND_CAN_NUM; i++)
            {
                _rym_putchar(ctx, RYM_CODE_CAN);
            }
            return err;
        }
            
//         if (err != RT_EOK)
//         {
// __ERROR_HANDLE:            
//             // rt_device_read(ctx->dev, 0, ctx->buf, _RYM_STX_PKG_SZ); // 清空错误缓冲区
//             if(errors++ > RYM_MAX_ERRORS)
//             {
//                       /* the spec require multiple CAN */
//                 for (i = 0; i < RYM_END_SESSION_SEND_CAN_NUM; i++)
//                 {
//                     _rym_putchar(ctx, RYM_CODE_CAN);
//                 }
//                 return err;/* Abort communication */
//             }
//             else
//                code = RYM_CODE_NAK;/* Ask for a packet */ 
//         }
//         else
//             errors = 0;

        switch (code)
        {
        case RYM_CODE_CAN:
            /* the spec require multiple CAN */
            for (i = 0; i < RYM_END_SESSION_SEND_CAN_NUM; i++)
            {
                _rym_putchar(ctx, RYM_CODE_CAN);
            }
            return -RYM_ERR_CAN;
        case RYM_CODE_ACK:
            _rym_putchar(ctx, RYM_CODE_ACK);
            break;
        // case RYM_CODE_NAK:
        //     _rym_putchar(ctx, RYM_CODE_NAK);
            break;
        default:
            // wrong code
            break;
        };
    }
}

static rt_err_t _rym_do_fin(struct rym_ctx *ctx)
{
    enum rym_code code;
    rt_uint16_t recv_crc;
    rt_size_t i;
    rt_size_t data_sz;

    ctx->stage = RYM_STAGE_FINISHING;
    /* we already got one EOT in the caller. invoke the callback if there is
     * one. */
    if (ctx->on_end)
        ctx->on_end(ctx, ctx->buf + 3, 128);

    _rym_putchar(ctx, RYM_CODE_NAK);
    code = _rym_read_code(ctx, RYM_WAIT_PKG_TICK);
    if (code != RYM_CODE_EOT)
        return -RYM_ERR_CODE;

    _rym_putchar(ctx, RYM_CODE_ACK);
    _rym_putchar(ctx, RYM_CODE_C);

    code = _rym_read_code(ctx, RYM_WAIT_PKG_TICK);
    if (code == RYM_CODE_SOH)
    {
        data_sz = _RYM_SOH_PKG_SZ;
    }
    else if (code == RYM_CODE_STX)
    {
        data_sz = _RYM_STX_PKG_SZ;
    }
    else
        return -RYM_ERR_CODE;

    i = _rym_read_data(ctx, _RYM_SOH_PKG_SZ - 1);
    if (i != (_RYM_SOH_PKG_SZ - 1))
        return -RYM_ERR_DSZ;

    /* sanity check
     */
    if (ctx->buf[1] != 0 || ctx->buf[2] != 0xFF)
        return -RYM_ERR_SEQ;

    recv_crc = (rt_uint16_t)(*(ctx->buf + _RYM_SOH_PKG_SZ - 2) << 8) | *(ctx->buf + _RYM_SOH_PKG_SZ - 1);
    if (recv_crc != CRC16(ctx->buf + 3, _RYM_SOH_PKG_SZ - 5))
        return -RYM_ERR_CRC;

    /*next file transmission*/
    if (ctx->buf[3] != 0)
    {
        if (ctx->on_begin && ctx->on_begin(ctx, ctx->buf + 3, data_sz - 5) != RYM_CODE_ACK)
            return -RYM_ERR_CAN;
        return RT_EOK;
    }

    /* congratulations, check passed. */
    ctx->stage = RYM_STAGE_FINISHED;

    /* put the last ACK */
    _rym_putchar(ctx, RYM_CODE_ACK);

    return RT_EOK;
}