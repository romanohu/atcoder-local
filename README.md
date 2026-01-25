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
```
aclogin
1. ブラウザからAtCoderにログイン
2. 開発者モード(F12)を開きApplication→cookies→https://atcoder.jpと辿る
3. 表の中の**REVEL_SESSION**という項目のValueをコピー
4. terminalで```aclogin```→コピーしたものをそのままペースト

---
> acc submitについて
> ```acc submit```を使うとエラーが出て提出できないことがある．これはAtCoder側の仕様変更によるものである．そのためojの以下の部分を修正する．
```sh 
code .venv/lib/python3.10/site-packages/onlinejudge/service/atcoder.py
```
```python
# 590~601行目を以下に書き換える
parsed_memory_limit = re.search(r'^(メモリ制限|Memory Limit): ([0-9.]+) (KiB|MiB)', memory_limit)

assert parsed_memory_limit

memory_limit_value = parsed_memory_limit.group(2)
memory_limit_unit = parsed_memory_limit.group(3)
if memory_limit_unit == 'KiB':
    memory_limit_byte = int(float(memory_limit_value) * 1000)
elif memory_limit_unit == 'MiB':
    memory_limit_byte = int(float(memory_limit_value) * 1000 * 1000)
else:
    assert False
```

---
**参考にした記事**
- [AtCoderで快適に戦うための環境を作ろう(note)](https://note.com/dev_onecareer/n/n673b1e040956)
- [AtCoder CLIにおいてログインできないときの対処法(qiita)](https://qiita.com/namonaki/items/16cda635dd7c34496aaa)