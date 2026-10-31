# 著作権・由来に関する通知

このリポジトリ全体は、個別ファイルに別の互換ライセンス表記がある場合を除き、
GNU General Public License version 2 only（`GPL-2.0-only`）で配布されます。

## Linux `igb`ドライバ

`igb-ss5000/`はLinuxカーネルの次の版を基にしています。

- tag: `v7.0`
- commit: `028ef9c96e96197026887c0f092424679298aae8`
- 元のパス: `drivers/net/ethernet/intel/igb/`

これらのファイルには、Linuxカーネルで付与されたSPDX識別子と、Intel
Corporationその他の原著作者による著作権表示が含まれています。それらの表示は
変更しません。`GPL-2.0+`と記載されたファイルは、GPL version 2の条件で本成果物に
含めています。

SS5000固有の変更点は`igb-ss5000/SS5000-PATCH.md`に記録しています。

## 機器固有コード

`led/`、`mgmt/`、`target-files/`、`wdt/`および関連ドキュメントは、実機で観測した
動作、公開されているLinuxインターフェース、元ファームウェアとの比較・解析結果を
基に、このプロジェクト向けに作成した実装です。

このリポジトリには、SAXAの元ファームウェア、元ファームウェアから取得したELF、
`rtkctl`、`sio`、Ubuntu ISO、ビルド済みカーネルモジュール、eMMCイメージは含まれません。
`rtkctl`と`sio`は利用者が適法に取得し、各自の責任で別途配置する必要があります。

製品名および商標は識別目的でのみ使用しています。このプロジェクトはSAXA株式会社、
Intel Corporation、Realtek Semiconductor Corp.による承認・提携を示すものではありません。
