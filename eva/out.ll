; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [17 x i8] c"Is X == 42?: %d\0A\00", align 1
@1 = private unnamed_addr constant [16 x i8] c"Is X > 42?: %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 42, i32* %x, align 4
  %x1 = load i32, i32* %x, align 4
  %tmpcmp = icmp eq i32 %x1, 42
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @0, i32 0, i32 0), i1 %tmpcmp)
  %x2 = load i32, i32* %x, align 4
  %tmpcmp3 = icmp ugt i32 %x2, 42
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @1, i32 0, i32 0), i1 %tmpcmp3)
  ret i32 0
}
