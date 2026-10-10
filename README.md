# fin-proto-runtime-bin-c

fin-proto 二进制协议 C11 运行时库,与
[fin-proto-runtime-bin-rs](https://github.com/xinchentechnote/fin-proto-runtime-bin-rs)(crate `binary-codec`)
和 [fin-proto-runtime-bin-zig](https://github.com/xinchentechnote/fin-proto-runtime-bin-zig) 语义对齐,供
[fin-protoc](https://github.com/xinchentechnote/fin-protoc) 生成的 C 消息代码依赖。

## 组成

- `include/binary_codec.h` + `src/binary_codec.c`:`fc_buffer`(编码缓冲)/`fc_reader`(解码游标,
  均携带字节序配置)、大/小端标量读写、定长字符数组(填充/去填充)、动态字符串、
  长度前缀列表(由生成代码循环展开)。
- `include/check_sum.h` + `src/check_sum.c`:`CRC16`(MODBUS)、`CRC32`(ISO-HDLC)、
  `SSE_BIN`(字节和 &0xFF)、`SZSE_BIN`(i32 字节和 mod 256)。

## 语义约定(与 Rust/Zig 版一致)

- 字节序由 `fc_buffer`/`fc_reader` 携带(DSL `LittleEndian` 选项),默认大端。
- 解码零拷贝:字符串以 `(const char *, size_t)` 视图借用输入缓冲;
  仅 `repeat` 字段通过 `calloc` 分配外层数组,配套生成的 `<msg>_free` 负责释放。
- 解码函数开始时先 `memset` 清零输出结构;失败时返回 `false`,调用方调用
  `<msg>_free` 即可安全释放已分配的部分(全零字段对 free 是安全的)。
- 定长 `char[N]` 编码超长截断、不足补空格(或 `@leftPad/@rightPad` 指定的填充字符)。

## 使用

生成的 C 代码通过头文件引用本库(编译时 `-I <runtime>/include` 并链接 `src/*.c`),
参考 [fin-proto-sse-bin-c](https://github.com/xinchentechnote/fin-proto-sse-bin-c) 等工程:

```bash
make test      # 编译并运行运行时自测
```
