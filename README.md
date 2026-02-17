# AtCoder環境構築
## 使用しているツール
- [atcoder-cli](https://github.com/Tatamo/atcoder-cli)
- [online-judge-tools](https://github.com/online-judge-tools/oj)
- [aclogin](https://github.com/key-moon/aclogin)

## セットアップ
ツールのインストール
```sh
uv sync
npm install -g atcoder-cli
```
atcoder-cliの設定
```sh
acc check-oj
acc config default-task-choice all
acc config default-template "$(pwd)/template/cpp"
acc config default-contest-dirname-format 'contests/{ContestID}'
acc config default-test-dirname-format test
```
aclogin
1. ブラウザからAtCoderにログイン
2. 開発者モード(F12)を開きApplication→cookies→https://atcoder.jp と辿る
3. 表の中の**REVEL_SESSION**という項目のValueをコピー
4. terminalで```aclogin```→コピーしたものをそのままペースト

---
> acc submitについて
使用できないらしい(厳密にはコンテスト開催時にだけ使用できる?)

---
**参考にした記事**
- [AtCoderで快適に戦うための環境を作ろう(note)](https://note.com/dev_onecareer/n/n673b1e040956)
- [AtCoder CLIにおいてログインできないときの対処法(qiita)](https://qiita.com/namonaki/items/16cda635dd7c34496aaa)