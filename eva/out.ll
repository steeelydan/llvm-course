; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [4 x i8] c"%d \00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 10, i32* %x, align 4
  br label %cond

cond:                                             ; preds = %body, %entry
  %x1 = load i32, i32* %x, align 4
  %tmpcmp = icmp ugt i32 %x1, 0
  br i1 %tmpcmp, label %body, label %loopend

body:                                             ; preds = %cond
  %x2 = load i32, i32* %x, align 4
  %tmpsub = sub i32 %x2, 1
  store i32 %tmpsub, i32* %x, align 4
  %x3 = load i32, i32* %x, align 4
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @0, i32 0, i32 0), i32 %x3)
  br label %cond

loopend:                                          ; preds = %cond
  ret i32 0
}
