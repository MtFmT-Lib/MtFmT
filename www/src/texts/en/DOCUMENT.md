Mtfmt is a formatter library. It's another implementation for the dialect of PEP-3101 and written in pure C language and optimized for the embedded system.

[toc]

# 1 Introduction

The mini template formatting library, or MtFmt is an effective, small formatter library written in pure C language. It implements a dialect for [PEP-3101](https://peps.python.org/pep-3101/ "PEP-3101"). In addition, it implements fixed-point number and quantized value formatting without the divide operator required and implements real-time formatting referencing the [.Net standard](https://learn.microsoft.com/zh-cn/dotnet/standard/base-types/standard-date-and-time-format-strings "Standard time formatting"). The main features are the following.

* Integer formatting supports binary, octal, decimal hexadecimal, and hexadecimal with prefix output
* Quantized value formatting
* Standard time formatting and user-defined time formatting
* String formatting
* Array formatting
* Specifiable align, the filling width, fill character, and sign display style
* Optional build-in heap manager for embedded system
* Optional specific build-in divide operator implementing
* Flexible syntax and clear error report.

# 2 Providing API

Formatting API includes four parts. The first one is the string API, which provides a dynamic string for the library. And the second one is the formatting API, which provides the formatting implements with `va_arg`. The third one is a scanner and the last one is an optional heap manager, it's designed for embedded systems without malloc and RTOS. The full documents of source codes are deployed at [HERE](https://mtfmt-lib.github.io/mtfmt/doxygen/html/ "Doxygen document").

## 2.1 Error handling

The resulting code, or `mstr_result_t` type, is a 32 bits unsigned integer.

## 2.2 String

The string API provides a dynamic string object, including optimizing the short string. The basic struct of that is the following.

```c
struct MString
{
    char* buff;
    usize_t length;
    usize_t cap_size;
    char stack_region[MSTR_STACK_REGION_SIZE];
};
```

As you see, the implementation has four properties mainly. The first one, `buff`, is the pointer of the string memory region. The second one, `length`, is the count of buff. It indicates how many bytes of the buffer. And the third one, `cap_size`, also called capacity, is the amount of space in the underlying character buffer. And with the help of property `stack_region`, the string can be allocated on the stack located in the currently active record. So the string is located in the stack when the `stack_region` is equal for the `buff`. Otherwise, it's located in the heap. The main motivation is assuming that the string usually contains few characters, so it can reduce the heap usage in usual. The following sections descript the export API and the design details of `MString`. For the encoding of the string will be controlled by macro `_MSTR_USE_UTF8`, which determines which encoding (UTF-8 or NOT UTF-8) will be used. See more about string encoding in [section 2.6](#section_2_6_Advanced_topics).

### 2.2.1 Allocator

The allocator is fixed and can be selected by macro `_MSTR_USE_MALLOC`. For the short string, which length is less than `MSTR_STACK_REGION_SIZE`,  the string will be allocated into the stack region. Otherwise, it will be allocated into the heap. The macro `_MSTR_USE_MALLOC` indicated which allocator should be used. When it's equal to 0, the built-in heap manager will be selected. Otherwise, the allocator is malloc, in `stdlib`. Normally, use the macro functions as follows to replace the malloc in your application. Those will be switched by the macro `_MSTR_USE_MALLOC`. See more details in [section 2.5](#section_2_5_Build_in_heap_manager).

* `mstr_heap_init`: Initialize the heap
* `mstr_heap_alloc`: Allocate memory
* `mstr_heap_free`: Release

### 2.2.2 Transformation

The transformation functions provide reverse string and clear string. The transformation function has the same input and output, it will be run in situ, and the evaluation result always is `MStr_Ok`. So that has no return value. The prototype of those is the following.

```c
void mstr_TRANS(MString* input_output);
```

As an example, the following code shows how to reverse a string using `mstr_reverse_self`. All transformation functions have the same usage.

```c
MString str;
mstr_create(&str, "Example");
mstr_reverse_self(&str);
// now, str == 'elpmaxE'
```

#### 2.2.2.1 Clear string

The clear function `mstr_clear` sets the `length` property to zero simply. It will make this object seem empty after this function is called.

For a string holds length $N$, this function implementation has $O(1)$ time complexity and spatial complexity.

#### 2.2.2.1 Reverse string

The reverse function `mstr_reverse_self` reverses all characters in the original string. This function will reverse one by one character rather than just reverse byte.

For a string holds length $N$, this function implementation has $O(N)$ time complexity and $O(1)$ spatial complexity.

### 2.2.3 Concatenating

The concatenating operator for string means that push one or more characters into the source string. 

### 2.2.4 Equal operator

Equal

### 2.2.5 Other functions

The other

## 2.3 Formatter

The formatter API includes two parts. The first part is the `mstr_format` function, which provides formatting inputs and appends the output into the string with variable arguments.

### 2.3.1 Formatter function

TODO

### 2.3.2 Error report

TODO

## 2.4 Scanner

TODO

### 2.4.1 Formatter function

TODO

### 2.4.2 Default value

TODO

### 2.4.3 Error report

TODO

## 2.5 Build-in heap manager

The build-in heap manager is an optional part and it will be compiled when the macro `_MSTR_USE_MALLOC` is zero.

### 2.5.1 Allocator functions

The macro function `mstr_heap_init` 

### 2.5.2 Fragment counter

TODO

## 2.6 Advanced topics

TODO

# 3 Syntax

The syntax is not context-free syntax, but it can be parsed by a top-down parser easily. For the input of the formatting string, the parser will match the replacement field. The replacement field describes how to process arguments. Consider the input as follows, it shows how to format `input_tm` into `output`.

```c
mstr_result_t result_code = mstr_format(
    "Today is {0:t:%yyyy-%MM-%dd}",
    &output,
    1,
    &input_tm
);
```

The first argument is a literal string.

## 3.1 Foundations for formal languages

The formal language in this section means a well-defined language. It's not a natural language in the real world. If you know how a parser work and know how to read a formal syntax, you can [skip this section](#section_3_2_Notational_conventions).

## 3.2 Notational conventions

TODO

# 4 Compile the library

TODO

## 4.1 The Compiler 101

TODO

## 4.2 Dependent files

TODO

# 5 Developing

TODO

# 6 See also

TODO
