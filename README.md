# SAXA SS5000 Linux hardware support

SAXA SS5000上でUbuntu Serverを動作させるために作成した、機器固有のハードウェア対応コードと設定です。

## 収録内容

- `igb-ss5000/`: Intel I210と内蔵RTL8367C L2スイッチを接続するためのLinux 7.0 `igb`改変
- `led/`: Nuvoton Super I/O経由のフロントLED低レベル制御
- `mgmt/`: USB管理ポートのリンク確立時にARPを通知する補助プログラム
- `target-files/`: L2スイッチ、LED、管理ポート用のユーザー空間ツール、設定、systemd unit
- `wdt/`: initramfs/dracutからNCT611xD watchdogを停止するフック
- `make_rtkctl_flat.py`: 保存済みベンダー`rtkctl`のVLAN即値を構成に合わせて生成するための解析・生成補助
- `SS5000-*.md`: 実機で確認した構成、導入、操作手順

## 対象と検証済み環境

- 機器: SAXA SS5000
- Ubuntu Server 26.04
- カーネル: `7.0.0-14-generic`
- `igb`の上流ベース: Linux `v7.0`、commit `028ef9c96e96197026887c0f092424679298aae8`
- 内蔵スイッチ接続: PCI `0000:02:00.0`のIntel I210からRTL8367CへSGMII/MDIO接続

カーネルABI、PCIトポロジ、Super I/Oの型番やレジスタ配置に依存します。別バージョンや別個体への適用前に、必ずコンソールと復旧手段を用意してください。

## `igb`モジュールのビルド例

対象カーネルと一致するヘッダを導入した環境で実行します。

```sh
make -C /lib/modules/$(uname -r)/build M="$PWD/igb-ss5000" modules
```

生成される`igb.ko`はGit管理しません。Secure Bootが有効な環境では、環境固有の鍵による署名も必要です。

## ベンダーバイナリについて

内蔵RTL8367Cの初期化には、元ファームウェアから保存した`rtkctl`を使用します。このリポジトリにはベンダーバイナリ、元ファームウェア、ISO、eMMCバックアップ、ビルド済みカーネルモジュールを含めません。

WDTフックは同様に元ファームウェア由来の`/usr/sbin/sio`を実行しますが、そのバイナリも収録しません。

## ライセンスと由来

`igb-ss5000/`はLinuxカーネルの`igb`ドライバを基にしており、各ファイルのSPDX表記およびLinuxカーネルのGPL-2.0条件に従います。それ以外の機器固有コードについては、明示的なライセンス付与を行っていません。現時点では非公開リポジトリでの保管と内部利用を前提とします。

## 非収録データ

次のものは意図的にGit管理対象から除外しています。

- Ubuntu/Linuxのヘッダおよびカーネルソース作業ツリー
- `.o`、`.ko`、生成されたELFなどのビルド成果物
- `rtkctl`、`sio`など元ファームウェア由来のバイナリ
- Ubuntu ISO、復旧用eMMCイメージ、装置ログ
- パスワード、SSH鍵、GitHub認証情報

