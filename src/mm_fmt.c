// SPDX-License-Identifier: LGPL-3.0
/**
 * @file    mm_fmt.c
 * @author  向阳 (hinata.hoshino@foxmail.com)
 * @brief   字符串格式化
 * @version 1.0
 * @date    2023-03-21
 *
 * @copyright Copyright (c) 向阳, all rights reserved.
 *
 */

#define MSTR_IMP_SOURCES 1

#include "mm_fmt.h"
#include "mm_typedef.h"

/**
 * @brief 将元素类型T转换为对应的数组类型值
 *
 */
#define AS_ARRAY_TYPE(t) \
    ((MStrFmtArgType)((uint32_t)(t) | MStrFmtArgType_Array_Bit))

//
// private:
//

static mstr_result_t
    process_replacement_field(char const**, MStrFmtParseResult*);
static mstr_result_t load_value(
    MStrFmtFormatArgument*, MStrFmtArgsContext*, usize_t, MStrFmtArgType
);
static mstr_result_t
    format_value(MString*, const MStrFmtParseResult*, const MStrFmtFormatArgument*);
static mstr_result_t
    format_array(MString*, const MStrFmtParseResult*, const MStrFmtFormatArgument*, const MStrFmtFormatArgument*);
static iptr_t array_get_item(const MStrFmtFormatArgument*, usize_t);
static mstr_result_t
    copy_to_output(MString*, const MStrFmtFormatDescript*, const MString*);
static mstr_result_t
    convert(MString*, const MStrFmtParseResult*, const MStrFmtFormatArgument*);
static mstr_result_t
    convert_string(MString*, iptr_t, const MStrFmtFormatSpec*);
static mstr_result_t
    convert_time(MString*, iptr_t, const MStrFmtFormatSpec*);
static mstr_result_t convert_int(
    MString*, int32_t, MStrFmtSignDisplay, MStrFmtFormatType
);
static mstr_result_t convert_uint(
    MString*, uint32_t, MStrFmtFormatType
);
static mstr_result_t convert_quat(
    MString*, int32_t, uint32_t, MStrFmtSignDisplay
);
static mstr_result_t convert_uquat(MString*, uint32_t, uint32_t);
static mstr_result_t fmt_type_as_integer_index(
    MStrFmtIntIndex*, MStrFmtFormatType
);

//
// public:
//

MSTR_EXPORT_API(mstr_result_t)
mstr_format(MString* res_str, const char* fmt, usize_t fmt_place, ...)
{
    mstr_result_t res;
    va_list ap;
    va_start(ap, fmt_place);
    res = mstr_vformat(fmt, res_str, fmt_place, &ap);
    va_end(ap);
    return res;
}

MSTR_EXPORT_API(mstr_result_t)
mstr_vformat(
    const char* fmt,
    MString* res_str,
    usize_t fmt_place,
    va_list* ap_ptr
)
{
    MStrFmtArgsContext context = {0};
    context.max_place = fmt_place;
    context.p_ap = (va_list*)ap_ptr;
    return mstr_context_format(res_str, fmt, &context);
}

MSTR_EXPORT_API(mstr_result_t)
mstr_context_format(
    MString* res_str, const char* fmt, MStrFmtArgsContext* ctx
)
{
    mstr_result_t result = MStr_Ok;
    // 处理格式化串
    if (ctx->max_place > MFMT_PLACE_MAX_NUM) {
        return MStr_Err_IndexTooLarge;
    }
    // else:
    while (!!*fmt && MSTR_SUCC(result)) {
        if (*fmt != '{' && *fmt != '}') {
            // 非格式化内容, 正常copy走
            char ch = *fmt;
            fmt += 1;
            result = mstr_append(res_str, ch);
        }
        else {
            // 解析格式化串
            uint32_t arg_id;
            MStrFmtArgClass arg_class;
            MStrFmtFormatArgument arg = {0};
            MStrFmtFormatArgument arg_attach = {0};
            MStrFmtParseResult parser_result;
            MSTR_AND_THEN(
                result, process_replacement_field(&fmt, &parser_result)
            );
            // 处理结果
            arg_id = parser_result.val.val.id;
            arg_class = parser_result.arg_class;
            switch (arg_class) {
            case MStrFmtArgClass_EscapeChar:
                // 转义字符, 直接append
                MSTR_AND_THEN(
                    result,
                    mstr_append(res_str, parser_result.val.escape_char)
                );
                break;
            case MStrFmtArgClass_Value:
                // 载入参数
                MSTR_AND_THEN(
                    result,
                    load_value(
                        &arg, ctx, arg_id, parser_result.val.val.typ
                    )
                );
                // 进行格式化
                MSTR_AND_THEN(
                    result, format_value(res_str, &parser_result, &arg)
                );
                break;
            case MStrFmtArgClass_Array:
                // 载入参数
                MSTR_AND_THEN(
                    result,
                    load_value(
                        &arg,
                        ctx,
                        arg_id,
                        AS_ARRAY_TYPE(parser_result.val.arr.ele_typ)
                    )
                );
                // (数组长度)
                MSTR_AND_THEN(
                    result,
                    load_value(
                        &arg_attach,
                        ctx,
                        arg_id + 1,
                        MStrFmtArgType_Uint32
                    )
                );
                // 格式化数组
                MSTR_AND_THEN(
                    result,
                    format_array(
                        res_str, &parser_result, &arg, &arg_attach
                    )
                );
                break;
            }
        }
    }
    return result;
}

/**
 * @brief 解析replacement field
 *
 * @param[inout] ppfmt: & 格式化串
 * @param[out] res_str: 结果输出
 * @param[out] parser_result: 解析结果
 *
 */
static mstr_result_t process_replacement_field(
    char const** ppfmt, MStrFmtParseResult* parser_result
)
{
    mstr_result_t result;
    const char* pfmt = *ppfmt;
    // 解析内容
    MStrFmtParserState* state;
    byte_t state_memory[MFMT_PARSER_STATE_SIZE];
    mstr_fmt_parser_init(state_memory, pfmt, &state);
    result = mstr_fmt_parse_goal(state, parser_result);
    // 增加offset
    if (MSTR_SUCC(result)) {
        pfmt += mstr_fmt_parser_end_position(state, pfmt);
    }
    *ppfmt = pfmt;
    return result;
}

/**
 * @brief 从可变参数里面载入值
 *
 * @param[out] arg: 返回值
 * @param[inout] ctx: Context
 * @param[in] arg_id: 参数id
 * @param[in] spec_type: 指定的参数值类型
 */
static mstr_result_t load_value(
    MStrFmtFormatArgument* arg,
    MStrFmtArgsContext* ctx,
    usize_t arg_id,
    MStrFmtArgType spec_type
)
{
    uint32_t max_place = (uint32_t)ctx->max_place;
    mstr_result_t result = MStr_Ok;
    MStrFmtFormatArgument* cache = ctx->cache;
    if (arg_id >= max_place) {
        return MStr_Err_InvaildArgumentID;
    }
    if (arg_id > 0 &&
        cache[arg_id - 1].type == MStrFmtArgType_Unknown) {
        // 上一个参数为NULL(MStrFmtArgType_Unknown),
        // 那么认为上一个参数未使用, 所以把它当成未使用的id
        // 坏坏c的va_arg只能顺序取 (ˉ▽ˉ；)...
        return MStr_Err_UnusedArgumentID;
    }
    // else:
    if (cache[arg_id].type == MStrFmtArgType_Unknown) {
        // 反正丢进来的东东按照iptr_t对齐 (～o￣3￣)～
        iptr_t fmt_arg = (iptr_t)va_arg(*ctx->p_ap, iptr_t);
        // 记录它
        cache[arg_id].value = fmt_arg;
        cache[arg_id].type = spec_type;
    }
    // 返回结果
    arg->value = cache[arg_id].value;
    arg->type = cache[arg_id].type;
    return result;
}

/**
 * @brief 格式化值
 *
 * @param[out] res_str: buff
 * @param[in] parser_result: 格式化描述
 * @param[in] arg: 值
 * @param[in] sz_arg: 数组大小
 *
 */
static mstr_result_t format_array(
    MString* res_str,
    const MStrFmtParseResult* parser_result,
    const MStrFmtFormatArgument* arg,
    const MStrFmtFormatArgument* sz_arg
)
{
    MString buff;
    mstr_result_t result;
    usize_t array_len, array_index;
    if (arg->type !=
        (parser_result->val.arr.ele_typ | MStrFmtArgType_Array_Bit)) {
        return MStr_Err_InvaildArgumentType;
    }
    else if (sz_arg->type != MStrFmtArgType_Uint32) {
        return MStr_Err_InvaildArgumentType;
    }
    // else:
    // 数组长度
    array_len = (usize_t)sz_arg->value;
    // 格式化数组中的每一个元素
    array_index = 0;
    result = mstr_create_empty(&buff);
    while (MSTR_SUCC(result) && array_index < array_len) {
        MStrFmtFormatArgument element;
        element.type = parser_result->val.arr.ele_typ;
        element.value = array_get_item(arg, array_index);
        // 格式化元素的值到 internal_buff
        MSTR_AND_THEN(result, convert(&buff, parser_result, &element));
        // 增加分隔符
        if (MSTR_SUCC(result) && array_index + 1 < array_len) {
            const char* split_beg = parser_result->val.arr.split_beg;
            const char* split_end = parser_result->val.arr.split_end;
            result =
                mstr_concat_cstr_slice(&buff, split_beg, split_end);
        }
        // 失败的break在下次循环开始时
        array_index += 1;
    }
    // 处理对齐和填充
    MSTR_AND_THEN(
        result,
        copy_to_output(res_str, &parser_result->val.val.spec, &buff)
    );
    // 返回
    mstr_free(&buff);
    return result;
}

/**
 * @brief 按照数组下标进行访问
 *
 * @param array: 数组
 * @param index: 下标位置
 * @return iptr_t: & array[index]
 */
static iptr_t array_get_item(
    const MStrFmtFormatArgument* array, usize_t index
)
{
    MStrFmtArgType type =
        (MStrFmtArgType)((uint32_t)(array->type) &
                         (uint32_t)(MStrFmtArgType_Array_Bit - 1));
    usize_t ele_size = 0;
    iptr_t element_ptr = (iptr_t)NULL;
    const void* ptr = NULL;
    switch (type) {
    case MStrFmtArgType_Int8: ele_size = sizeof(int8_t); break;
    case MStrFmtArgType_Int16: ele_size = sizeof(int16_t); break;
    case MStrFmtArgType_Int32: ele_size = sizeof(int32_t); break;
    case MStrFmtArgType_Uint8: ele_size = sizeof(uint8_t); break;
    case MStrFmtArgType_Uint16: ele_size = sizeof(uint16_t); break;
    case MStrFmtArgType_Uint32: ele_size = sizeof(uint32_t); break;
    case MStrFmtArgType_CString: ele_size = sizeof(const char*); break;
    case MStrFmtArgType_Time: ele_size = sizeof(const MStrTime*); break;
    case MStrFmtArgType_QuantizedValue:
        ele_size = sizeof(int32_t);
        break;
    case MStrFmtArgType_QuantizedUnsignedValue:
        ele_size = sizeof(uint32_t);
        break;
    default: mstr_unreachable(); break;
    }
    // 取得值
    ptr = (const void*)(index * ele_size + (uptr_t)array->value);
    switch (type) {
    case MStrFmtArgType_Int8:
        element_ptr = (iptr_t)(*(const int8_t*)ptr);
        break;
    case MStrFmtArgType_Int16:
        element_ptr = (iptr_t)(*(const int16_t*)ptr);
        break;
    case MStrFmtArgType_Int32:
        element_ptr = (iptr_t)(*(const int32_t*)ptr);
        break;
    case MStrFmtArgType_Uint8:
        element_ptr = (iptr_t)(*(const uint8_t*)ptr);
        break;
    case MStrFmtArgType_Uint16:
        element_ptr = (iptr_t)(*(const uint16_t*)ptr);
        break;
    case MStrFmtArgType_Uint32:
        element_ptr = (iptr_t)(*(const uint32_t*)ptr);
        break;
    case MStrFmtArgType_CString:
        element_ptr = (iptr_t)(*(const char* const*)ptr);
        break;
    case MStrFmtArgType_Time:
        element_ptr = (iptr_t)(*(const MStrTime* const*)ptr);
        break;
    case MStrFmtArgType_QuantizedValue:
        element_ptr = (iptr_t)(*(const int32_t*)ptr);
        break;
    case MStrFmtArgType_QuantizedUnsignedValue:
        element_ptr = (iptr_t)(*(const int32_t*)ptr);
        break;
    default: mstr_unreachable(); break;
    }
    return element_ptr;
}

/**
 * @brief 格式化值
 *
 * @param[out] res_str: buff
 * @param[in] parser_result: 格式化描述
 * @param[in] arg: 值
 *
 */
static mstr_result_t format_value(
    MString* res_str,
    const MStrFmtParseResult* parser_result,
    const MStrFmtFormatArgument* arg
)
{
    MString buff;
    mstr_result_t result;
    if (arg->type != parser_result->val.val.typ) {
        return MStr_Err_InvaildArgumentType;
    }
    // else:
    // 按照value_type,
    // sign_display和fmt_type先格式化到mfmt_static_buff里面
    result = mstr_create_empty(&buff);
    MSTR_AND_THEN(result, convert(&buff, parser_result, arg));
    if (result == MStr_Err_BufferTooSmall) {
        result = MStr_Err_InternalBufferTooSmall;
    }
    // 处理对齐和填充
    MSTR_AND_THEN(
        result,
        copy_to_output(res_str, &parser_result->val.val.spec, &buff)
    );
    // 返回
    mstr_free(&buff);
    return result;
}

/**
 * @brief 把数据复制到输出
 *
 * @param[inout] out_str: & 转换输出的buff
 * @param[in] pout_end: buff大小
 * @param[in] fmt_spec: 格式化解析出来的信息
 * @param[in] src_str: 需要复制的字符串
 *
 */
static mstr_result_t copy_to_output(
    MString* out_str,
    const MStrFmtFormatDescript* fmt_spec,
    const MString* src_str
)
{
    usize_t src_len = 0;
    usize_t fill_len = 0;
    usize_t offset_len = 0;
    usize_t need_width = 0;
    mstr_result_t result = MStr_Ok;
    MStrFmtAlign align;
    char fill_char;
    if (fmt_spec->width == -1) {
        // 压根没有指定宽度, 不管对齐了
        return mstr_concat(out_str, src_str);
    }
    // 计算宽度够不够
    src_len = src_str->count;
    need_width = (usize_t)fmt_spec->width;
    if (src_len >= need_width) {
        // 宽度太宽, 不管对齐了
        return mstr_concat(out_str, src_str);
    }
    // 宽度确定足够, 按照align去copy, 并且用fill_char填充
    align = fmt_spec->fmt_align;
    fill_char = fmt_spec->fill_char;
    switch (align) {
    case MStrFmtAlign_Left:
        // 复制开头的内容
        MSTR_AND_THEN(result, mstr_concat(out_str, src_str));
        // 进行填充
        fill_len = need_width - src_len;
        MSTR_AND_THEN(
            result, mstr_repeat_append(out_str, fill_char, fill_len)
        );
        break;
    case MStrFmtAlign_Right:
        fill_len = need_width - src_len;
        MSTR_AND_THEN(
            result, mstr_repeat_append(out_str, fill_char, fill_len)
        );
        // 复制剩下的内容
        MSTR_AND_THEN(result, mstr_concat(out_str, src_str));
        break;
    case MStrFmtAlign_Center:
        fill_len = (need_width - src_len) / 2;
        // 左侧的填充
        MSTR_AND_THEN(
            result, mstr_repeat_append(out_str, fill_char, fill_len)
        );
        // 中间的内容
        MSTR_AND_THEN(result, mstr_concat(out_str, src_str));
        // 右边的填充
        offset_len = fill_len + src_len;
        fill_len = need_width - offset_len;
        MSTR_AND_THEN(
            result, mstr_repeat_append(out_str, fill_char, fill_len)
        );
        break;
    }
    return result;
}

/**
 * @brief 转换单个的值到buffer
 *
 * @param[out] str: 转换输出
 * @param[in] parser_result: parser结果
 * @param[in] arg: 需要转换的值
 *
 */
static mstr_result_t convert(
    MString* str,
    const MStrFmtParseResult* parser_result,
    const MStrFmtFormatArgument* arg
)
{
    iptr_t value = arg->value;
    MStrFmtArgType value_type = arg->type;
    mstr_result_t result;
    switch (value_type) {
    case MStrFmtArgType_Uint8:
    case MStrFmtArgType_Uint16:
    case MStrFmtArgType_Uint32:
        result = convert_uint(
            str,
            (uint32_t)value,
            parser_result->val.val.spec.fmt_spec.fmt_type
        );
        break;
    case MStrFmtArgType_Int8:
    case MStrFmtArgType_Int16:
    case MStrFmtArgType_Int32:
        result = convert_int(
            str,
            (int32_t)value,
            parser_result->val.val.spec.sign_display,
            parser_result->val.val.spec.fmt_spec.fmt_type
        );
        break;
    case MStrFmtArgType_CString:
        result = convert_string(
            str, value, &parser_result->val.val.spec.fmt_spec
        );
        break;
    case MStrFmtArgType_Time:
        result = convert_time(
            str, value, &parser_result->val.val.spec.fmt_spec
        );
        break;
    case MStrFmtArgType_QuantizedValue:
        result = convert_quat(
            str,
            (int32_t)value,
            parser_result->val.val.prop.a,
            parser_result->val.val.spec.sign_display
        );
        break;
    case MStrFmtArgType_QuantizedUnsignedValue:
        result = convert_uquat(
            str, (uint32_t)value, parser_result->val.val.prop.a
        );
        break;
    default: result = MStr_Err_UnsupportType; break;
    }
    return result;
}

/**
 * @brief 对字符串格式化
 *
 */
static mstr_result_t convert_string(
    MString* str, iptr_t value, const MStrFmtFormatSpec* spec
)
{
    if (spec->fmt_type != MStrFmtFormatType_UnSpec) {
        return MStr_Err_UnsupportFormatType;
    }
    else {
        const char* src_str = (const char*)value;
        return mstr_concat_cstr(str, src_str);
    }
}

/**
 * @brief 对日期和时间值格式化
 *
 */
static mstr_result_t convert_time(
    MString* str, iptr_t value, const MStrFmtFormatSpec* spec
)
{
    if (spec->fmt_type != MStrFmtFormatType_UnSpec) {
        return MStr_Err_UnsupportFormatType;
    }
    else {
        const MStrTime* tm = (const MStrTime*)value;
        return mstr_fmt_ttoa(str, tm, &spec->spec.chrono);
    }
}

/**
 * @brief 对有符号量化值进行格式化
 *
 */
static mstr_result_t convert_quat(
    MString* str, int32_t value, uint32_t qbits, MStrFmtSignDisplay sign
)
{
    return mstr_fmt_iqtoa(str, value, qbits, sign);
}

/**
 * @brief 对无符号量化值进行格式化
 *
 */
static mstr_result_t convert_uquat(
    MString* str, uint32_t value, uint32_t qbits
)
{
    return mstr_fmt_uqtoa(str, value, qbits);
}

/**
 * @brief 对有符号整数进行格式化
 *
 */
static mstr_result_t convert_int(
    MString* str,
    int32_t value,
    MStrFmtSignDisplay sign,
    MStrFmtFormatType ftyp
)
{
    MStrFmtIntIndex index;
    mstr_result_t result = MStr_Ok;
    // 取得对应的index
    MSTR_AND_THEN(result, fmt_type_as_integer_index(&index, ftyp));
    // 进行格式化
    MSTR_AND_THEN(result, mstr_fmt_itoa(str, value, index, sign));
    return result;
}

/**
 * @brief 对无符号整数进行格式化
 *
 */
static mstr_result_t convert_uint(
    MString* str, uint32_t value, MStrFmtFormatType ftyp
)
{
    MStrFmtIntIndex index;
    mstr_result_t result = MStr_Ok;
    // 取得对应的index
    MSTR_AND_THEN(result, fmt_type_as_integer_index(&index, ftyp));
    // 进行格式化
    MSTR_AND_THEN(result, mstr_fmt_utoa(str, value, index));
    return result;
}

/**
 * @brief 把 MStrFmtFormatType 转为 整数的index
 *
 * @param[out] index: 结果
 * @param[in] typ: MStrFmtFormatType
 */
static mstr_result_t fmt_type_as_integer_index(
    MStrFmtIntIndex* index, MStrFmtFormatType typ
)
{
    mstr_result_t result = MStr_Ok;
    MStrFmtIntIndex index_map_result = MStrFmtIntIndex_Dec;
    switch (typ) {
    case MStrFmtFormatType_Binary:
        index_map_result = MStrFmtIntIndex_Bin;
        break;
    case MStrFmtFormatType_Oct:
        index_map_result = MStrFmtIntIndex_Oct;
        break;
    case MStrFmtFormatType_Deciaml:
        index_map_result = MStrFmtIntIndex_Dec;
        break;
    case MStrFmtFormatType_Hex:
        index_map_result = MStrFmtIntIndex_Hex;
        break;
    case MStrFmtFormatType_Hex_UpperCase:
        index_map_result = MStrFmtIntIndex_Hex_UpperCase;
        break;
    case MStrFmtFormatType_Hex_WithPrefix:
        index_map_result = MStrFmtIntIndex_Hex_WithPrefix;
        break;
    case MStrFmtFormatType_Hex_UpperCase_WithPrefix:
        index_map_result = MStrFmtIntIndex_Hex_UpperCase_WithPrefix;
        break;
    case MStrFmtFormatType_UnSpec:
        // 默认是十进制
        index_map_result = MStrFmtIntIndex_Dec;
        break;
    }
    *index = index_map_result;
    return result;
}
