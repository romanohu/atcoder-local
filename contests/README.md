# コマンド
## acc
コンテストのインストール
```sh
acc new {contest_name}
```
`tools/acc-wrapper.zsh` を読み込んでいる場合は、同時に `memo.md` も自動生成される。
テストの再インストール
```sh
# contests/<contest_name>で
acc add --choice all --no-template -f
```
書いたコードのテスト(<test_number>.outが必要)
```sh
uv run oj test
```
コンテストの提出
```sh
# 提出するファイルがあるディレクトリで
acc submit
# or
acc s
```
## g++
コンパイル
```sh
g++ -o a.out main.cpp
```
