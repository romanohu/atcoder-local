# AtCoder ローカル開発環境

## 概要

このリポジトリは、AtCoder の C++ 解答をローカルで作成、ビルド、実行、サンプルテスト、提出するための環境です。インストール可能な `acc` コマンドで `atcoder-cli` を拡張し、ローカル開発用のコマンドを提供します。

## 必要なもの

- Git
- [uv](https://docs.astral.sh/uv/)
- Node.js と npm（`atcoder-cli` のインストールに使用）
- C++23 に対応した C++ コンパイラ
- bash または zsh
- AtCoder アカウント

GNU GCC 15 を推奨します。別の C++23 対応コンパイラもビルドには使用できますが、`acc doctor` は警告を表示します。環境に GNU GCC が存在するかは `acc doctor` とコンパイラの `--version` で確認してください。

## セットアップ

リポジトリをクローンし、リポジトリルートで依存関係をインストールします。

```sh
git clone <url>
cd atcoder-local
uv sync --group dev
npm install -g atcoder-cli@2.2.0
uv tool install --editable .
```

`uv sync --group dev` は `online-judge-tools`、`oj-bundle`、`aclogin` とテスト用の `pytest` を仮想環境へインストールします。`atcoder-cli` は npm のグローバルパッケージとしてインストールします。`uv tool install --editable .` の後は、新しいシェルでもすぐに `acc` を使えます。

パッケージのメタデータまたは依存関係を更新した後は、次を実行してインストール済みコマンドを更新します。

```sh
uv tool install --force --editable .
```

## atcoder-cli の設定

リポジトリルートで次を実行します。テンプレートの設定値には、このクローンの絶対パスが保存されます。

```sh
uv run acc check-oj
acc config default-task-choice all
acc config default-template "$(pwd)/template/cpp"
acc config default-contest-dirname-format 'contests/{ContestID}'
acc config default-test-dirname-format test
```

AtCoder へのログイン情報は `aclogin` で設定します。ブラウザで AtCoder にログインし、開発者ツールの Cookie 一覧から `REVEL_SESSION` の値を確認して、次のコマンドの入力欄へ貼り付けます。Cookie は秘密情報として扱い、ファイルやシェル履歴へ保存しないでください。

```sh
uv run aclogin
```

## bash / zsh の互換用ラッパー

通常は前節の `uv tool install --editable .` を使用してください。既存のシェル設定との互換性のために、使用するシェルに合わせてリポジトリルートでどちらか一方を `source` することもできます。

zsh:

```sh
source "$(pwd)/tools/acc-wrapper.zsh"
```

bash:

```sh
source "$(pwd)/tools/acc-wrapper.bash"
```

ラッパーは現在のシェルセッションだけで有効です。セットアップコマンドとラッパーは `.zshrc` や `.bashrc` を自動では変更しません。常時使用する場合は、ユーザーが対応する `source` 行を `~/.zshrc` または `~/.bashrc` へ手動で追加し、`$(pwd)` ではなくこのクローンの絶対パスを指定してください。インストール済みの `acc` とこの互換用ラッパーを同じシェルで併用しないでください。

## `acc doctor`

インストール済みの `acc` で `acc doctor` を実行します。互換用ラッパーを使用する場合も、同じコマンドで診断できます。

```sh
acc doctor
```

`acc doctor` は `uv`、`atcoder-cli`、`oj`、`oj-bundle`、C++23 コンパイラ、同梱 ACL とローカルヘッダー、インストール済みの `acc` または互換用ラッパーの有効化を順に確認します。`[error]` があれば終了コード 1、GCC 15 以外の利用など `[warning]` だけであれば終了コード 0 です。

## コンテストの作成

リポジトリルートで、コンテスト ID を指定します。

```sh
acc new abc123
```

`acc new` は、指定した引数をそのまま `atcoder-cli` へ渡してコンテストを作成します。

## ビルド、実行、テスト、デバッグ、提出

通常は `contest.acc.json` に登録されたタスクディレクトリ、たとえば `contests/abc123/a` で実行します。

```sh
acc build
acc run
acc test
acc test --debug
acc submit
```

- `acc build` は C++23 のリリース設定で `main.cpp` をビルドします。ダウンロード済みサンプルは必要ありません。
- `acc run` はリリースビルド後にプログラムを実行します。ダウンロード済みサンプルは必要ありません。
- `acc test` と `acc test --debug` は `main.cpp` とローカルヘッダーを `.submission.cpp.pending` へ bundle し、その単一ファイルを各モードでコンパイルしてサンプルを実行します。サンプルがすべて成功した後、各タスクディレクトリの `submission.cpp` として公開されます。サンプルテストがない場合は、bundle とコンパイルの成功後に警告を表示し、サンプルテストを省略して `submission.cpp` を公開します。
- `acc test --debug` では `LOCAL`、AddressSanitizer、UndefinedBehaviorSanitizer を有効にします。
- `acc submit` は bundle、コンパイル、サンプルテストを実行しません。先に `acc test` を実行して生成した、各タスクディレクトリの新しい `submission.cpp` だけを確認後に提出します。

`acc submit` は対話端末専用です。確認は既定で No であり、`y` または `yes` を明示的に入力した場合だけ `atcoder-cli` へ提出を渡します。空入力、その他の入力、確認の中断では提出しません。

ビルドバイナリは `.atcoder-local/build/` 配下に置かれ、Git の管理対象にはなりません。生成された `submission.cpp` と `.submission.cpp.pending` は `contests/` 配下で Git の管理対象から除外されます。`main.cpp` または `library/` 配下の任意のファイルを変更すると `submission.cpp` は古い状態になるため、提出前にもう一度 `acc test` を実行してください。

## リポジトリルートからの `-c` / `-t` 指定

リポジトリルートなどタスクディレクトリ以外から実行する場合は、コンテスト ID とタスクラベルを両方指定します。片方だけの指定はエラーです。タスクラベルは大文字・小文字を区別しません。

```sh
acc build -c abc123 -t a
acc run -c abc123 -t a
acc test -c abc123 -t a
acc test --debug -c abc123 -t a
acc submit -c abc123 -t a
```

## CAPTCHA による提出制限

AtCoder はソースコード提出に CAPTCHA を導入しています。公式告知では、開催中の ABC、ARC、AGC、AHC などの Rated コンテストは CAPTCHA なしで提出できる扱いですが、この仕様は将来にわたって保証されていません。開催中の対象コンテスト以外など、AtCoder がその時点で CLI 提出を許可する条件から外れる場合、非公式ツールである `atcoder-cli` の提出は失敗することがあります。最新情報は [AtCoder の公式告知](https://atcoder.jp/posts/1456?lang=ja) を確認してください。

このリポジトリの `acc submit` は、提出失敗時にブラウザを開きません。提出 URL の表示、URL やソースのクリップボードへのコピー、Web 提出などの代替経路への切り替えも行いません。失敗時はエラーをそのまま返すため、提出済みと判断せず AtCoder 上の提出一覧を確認してください。

## トラブルシューティング

- `acc doctor` が `current shell is not using the repository wrapper` と表示する場合は、インストール済みの `acc` を使用しているか確認してください。互換用ラッパーを選んでいる場合は、使用中のシェルに対応する `tools/acc-wrapper.zsh` または `tools/acc-wrapper.bash` をもう一度 `source` してください。
- `no C++23 compiler found` の場合は C++23 対応コンパイラをインストールし、必要ならその実行ファイルを `CXX` に指定してから再診断してください。コンパイラが未導入の環境で GNU GCC が利用できるとは仮定しないでください。
- `oj-bundle` が GNU C++ コンパイラを要求して失敗する場合は、GNU GCC の導入状況と `CXX` を確認してから `acc test` を再実行してください。
- リポジトリルートで `acc build` などがタスクを特定できない場合は、`-c` と `-t` を両方指定してください。
- `acc submit` が提出前に停止する場合は、`submission.cpp` の有無と新しさ、対話端末、確認入力を確認し、必要なら `acc test` を再実行してください。AtCoder への提出が失敗した場合は、ログイン状態、CAPTCHA の条件、AtCoder 側の提出履歴を順に確認してください。このラッパーは自動的な代替提出を行いません。
