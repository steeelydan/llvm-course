; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 42, align 4

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  ret i32 0
}
