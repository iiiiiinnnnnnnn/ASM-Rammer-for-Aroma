# ASM Rammer

Aroma環境で動作するシンプルなメモリ操作プラグインです。
もともとは TCP Gecko の移植を目指していましたが、面倒になったので、**アセンブリ書き込み専用の簡易版**として仕上げました。

## 概要

このプラグインは、指定したメモリアドレスに対して、任意の値を書き込んだり読み取ったりすることができます。

## 必要環境・ビルドに必要なもの

以下のライブラリが必要です：

* [WUPS](https://github.com/wiiu-env/WiiUPluginSystem)
* [WUMS](https://github.com/wiiu-env/WiiUModuleSystem)
* [WUT](https://github.com/devkitPro/wut)

**devkitPro環境**が構築されている前提で、上記をセットアップすることでビルドできます。

```bash
make
```

で `asm_rammer.wps` が生成されます。

## 導入方法

1. このリポジトリをビルドして、`asm_rammer.wps` を生成します。
2. SDカード内の `sd:/wiiu/environments/aroma/plugins/` に配置してください。
3. Aromaメニューからプラグインを有効化します。
4. メニュー画面に表示される IPv4 アドレスとポート番号に基づき、クライアントソフトから接続します。

## コマンド一覧

接続後、以下のようなコマンドを送信することでメモリ操作が可能です。

```
w 0xAAAAAAAA 0xVVVVVVVV
```

* アドレス `0xAAAAAAAA` に 32bit 値 `0xVVVVVVVV` を書き込みます。

```
r 0xAAAAAAAA
```

* アドレス `0xAAAAAAAA` から 32bit 値を読み取ります。

## 補足

* 現在はこの基本的な機能のみ対応しています。
* 今後の開発については**気が向いたら**進めます。

## クライアントソフト

※↓未公開
Windows向けに専用クライアントソフト「**ASM Rammer PC**」も用意されています。
GUIで上記コマンドを簡単に送信できます。
