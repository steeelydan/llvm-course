; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [14 x i8] c"Version: %d\0A\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %VERSION = load i32, i32* @VERSION, align 4
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @0, i32 0, i32 0), i32 %VERSION)
  ret i32 0
}
