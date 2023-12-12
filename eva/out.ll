; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [7 x i8] c"X: %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %VERSION = alloca i32, align 4
  store i32 42, i32* %VERSION, align 4
  %z = alloca i32, align 4
  store i32 32, i32* %z, align 4
  %z1 = load i32, i32* %z, align 4
  %tmpadd = add i32 %z1, 11
  %x = alloca i32, align 4
  store i32 %tmpadd, i32* %x, align 4
  %x2 = load i32, i32* %x, align 4
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @0, i32 0, i32 0), i32 %x2)
  ret i32 0
}
