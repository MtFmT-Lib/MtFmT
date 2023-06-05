// SPDX-License-Identifier: LGPL-3.0
/**
 * @file    main.h
 * @author  向阳 (hinata.hoshino@foxmail.com)
 * @brief   所有可用的测试
 * @version 1.0
 * @date    2023-06-03
 *
 * @copyright Copyright (c) 向阳, all rights reserved.
 *
 */
#if !defined(_INCLUDE_MAIN_H_)
#define _INCLUDE_MAIN_H_ 1

#if __cplusplus
extern "C"
{
#endif
    void allocate_then_free(void);

    void fmt_align_left(void);
    void fmt_align_left_fill(void);
    void fmt_align_left_long(void);
    void fmt_align_right(void);
    void fmt_align_right_fill(void);
    void fmt_align_right_long(void);
    void fmt_align_middle(void);
    void fmt_align_middle_fill(void);
    void fmt_align_middle_long(void);

    void fmt_seq_arg_id(void);
    void fmt_seq_arg_id_err(void);

    void fmt_quat_value_sign(void);
    void fmt_quat_value_dualprec(void);
    void fmt_quat_value_singprec(void);

    void fmt_behav_signed_bin(void);
    void fmt_behav_signed_oct(void);
    void fmt_behav_signed_dec(void);
    void fmt_behav_signed_hex(void);

    void fmt_sign_add(void);
    void fmt_sign_sub(void);
    void fmt_sign_space(void);

    void fmt_sty_bin(void);
    void fmt_sty_oct(void);
    void fmt_sty_dec(void);
    void fmt_sty_hex(void);
    void fmt_sty_hex_prefix(void);

    void append_char(void);
    void equal_string(void);
    void reverse_self(void);
    void repeat_append_char(void);

    void fmt_array(void);
    void fmt_array_element_style(void);
    void fmt_array_userdefind_split(void);

    void fmt_chrono_default_f(void);
    void fmt_chrono_default_g(void);
    void fmt_chrono_userdef_year(void);
    void fmt_chrono_userdef_month(void);
    void fmt_chrono_userdef_day(void);
    void fmt_chrono_userdef_hour(void);
    void fmt_chrono_userdef_minute(void);
    void fmt_chrono_userdef_second(void);
    void fmt_chrono_userdef_subsecond(void);
    void fmt_chrono_userdef_week(void);

    void fmt_escape_bracket(void);

    void cpp_string_equal(void);
    void cpp_string_not_equal(void);
    void cpp_string_push(void);
    void cpp_string_concat(void);

    void cpp_monadic_result_or_value(void);
    void cpp_monadic_result_or_exception(void);

    void cpp_wrap_fmt(void);
#if __cplusplus
}
#endif
#endif // _INCLUDE_MAIN_H_
