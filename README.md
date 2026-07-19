# AtCoder環境構築
## 使用しているツール
- [atcoder-cli](https://github.com/Tatamo/atcoder-cli)
- [online-judge-tools](https://github.com/online-judge-tools/oj)
- [aclogin](https://github.com/key-moon/aclogin)

## セットアップ
クローン
```sh
git clone <url>
# 既存のcontests/ が不要なら
rm -rf ./contests
```

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

## コンテスト中のpush防止

### 初回設定

リポジトリのルートで次を実行します。

```sh
uv run python tools/push_guard.py install
source "$(pwd)/tools/acc-wrapper.zsh"
```

`install` はクローンごとに一度だけ必要です。現在有効な `core.hooksPath` が別の場所を向いている場合はエラーになり、既存の設定は書き換えません。

`source` はシェルを開くたびに実行します。常に有効にする場合は、リポジトリの絶対パスを使ってシェルの起動設定に追加してください。このラッパーを通さない `acc new` では、コンテスト時刻を自動登録できません。

### 動作

`acc new abc467` が成功すると、ラッパーがAtCoderの公式ページから開始時刻と終了時刻を取得し、Gitの管理領域に保存します。`pre-push` フックは保存済みの時刻だけを読み、ネットワークには接続しません。

登録したコンテストの期間 `[start_at, end_at)` は、リモートやブランチにかかわらずpushを止めます。開始時刻より前と、終了時刻ちょうどまたはそれ以降は自動でpushできるようになります。複数のコンテストを登録した場合は、開催中または終了時刻を確定できていないものが1件でもあればpushを止めます。

### 取得失敗と復旧

公式時刻の取得や解析に失敗すると、ラッパーは入力を求める前に、現在時刻から有効になる未解決のロックを保存します。対話端末ではJSTの終了時刻を一度だけ入力できます。入力が不正な場合、入力を中断した場合、非対話実行の場合は未解決のまま残り、pushも引き続き止まります。

状態の確認と復旧には次のコマンドを使います。

```sh
uv run python tools/push_guard.py status
uv run python tools/push_guard.py set-end abc467 "2026-07-18 22:40"
uv run python tools/push_guard.py recover-state abc467 "2026-07-18 22:40"
```

`status` は、フックがインストール済みかどうかと、登録した各コンテストの状態（`upcoming`、`active`、`expired`、`unresolved`）を表示します。

`set-end` は、登録済みで終了時刻が未解決のコンテストにだけ使えます。指定する時刻は未来のJSTです。

`recover-state` は、状態ファイルが壊れて読み込めない場合にだけ使えます。状態ファイルが壊れている間もpushは止まります。復旧時は壊れたファイルを時刻付きの別ファイルへバックアップしてから、現在時刻から指定した終了時刻まで有効なロックで置き換えます。

### 対象と限界

リポジトリの `pre-push` フックを実行する通常のGit pushと、IDEからのpushが対象です。リモート名やブランチ名による除外はありません。

`git push --no-verify`、`core.hooksPath`やフックの削除、`.git`内の状態ファイルの編集では回避できます。ローカルの設定を意図的に変更する操作までは防ぎません。

## `memo.md` の自動生成

上記のラッパーには、コンテスト時刻の登録に加えて `memo.md` を作る処理も含まれています。有効化後に `acc new abc454` を実行すると、`contests/<contest-id>/memo.md` に次の形式でテンプレートが作成されます。

```md
# abc454

## a

## b
...
```

---
> acc submitについて
使用できないらしい(厳密にはコンテスト開催時にだけ使用できる?)

---
**参考にした記事**
- [AtCoderで快適に戦うための環境を作ろう(note)](https://note.com/dev_onecareer/n/n673b1e040956)
- [AtCoder CLIにおいてログインできないときの対処法(qiita)](https://qiita.com/namonaki/items/16cda635dd7c34496aaa)
