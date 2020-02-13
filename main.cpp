//--------------------------------------------------
//
//		TCP 1on1チャットプログラム
//
//--------------------------------------------------
#include <winsock2.h>
#include <process.h>
#include "wsock32error.h"
#include "resource.h"

//--------------------------------------------------
// マクロ
//--------------------------------------------------
#define PORTNO		(20250)
#define NAME		"MainWindow"
#define TITLE		"TCP 1on1チャット"
#define RECV_MAX	(1024)

//--------------------------------------------------
// ライブラリ
//--------------------------------------------------
#pragma	comment(lib,"ws2_32.lib")
#pragma	comment(lib,"winmm.lib")

//--------------------------------------------------
// プロトタイプ
//--------------------------------------------------
LRESULT CALLBACK WndProc(	// ウィンドウプロシージャ
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	);

BOOL CALLBACK DlgProc(		// ダイアログプロシージャ
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	);

void AcceptProc(void* p);	// 待受スレッド関数
void RecvProc(void* p);		// 受信スレッド関数
void AddLog(				// チャットログ追加
	const char* name,
	const char* text
	);

//--------------------------------------------------
// グローバル変数
//--------------------------------------------------
SOCKET g_waitsock = INVALID_SOCKET;	// 待受用ソケット(サーバーのみ使用)
SOCKET g_chatsock = INVALID_SOCKET;	// 通信用ソケット

HWND g_hDlg;				// ダイアログウィンドウハンドル
char g_recvbuf[RECV_MAX];	// 受信バッファ

//--------------------------------------------------
// WinMain関数
//--------------------------------------------------
int APIENTRY WinMain(
	HINSTANCE 	hInstance, 		// アプリケーションインスタンス値
	HINSTANCE 	hPrevInstance,	// 意味なし
	LPSTR 		lpszArgs, 		// 起動時の引数文字列
	int 		nWinMode		// ウインドウ表示モード
	)
{
	HWND			hwnd;						// ウインドウハンドル
	MSG				msg;						// メッセージ構造体
	WNDCLASSEX		wcex;						// ウインドウクラス構造体

	// ウインドウクラス情報のセット
	wcex.hInstance		= hInstance;			// インスタンス値のセット
	wcex.lpszClassName	= NAME;					// クラス名
	wcex.lpfnWndProc	= (WNDPROC)WndProc;		// ウインドウメッセージ関数
	wcex.style			= 0;					// ウインドウスタイル
	wcex.cbSize 		= sizeof(WNDCLASSEX);	// 構造体のサイズ
	wcex.hIcon			= LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);	// ラージアイコン
	wcex.hIconSm		= LoadIcon((HINSTANCE)NULL, IDI_WINLOGO);		// スモールアイコン
	wcex.hCursor		= LoadCursor((HINSTANCE)NULL, IDC_ARROW);		// カーソルスタイル
	wcex.lpszMenuName	= 0; 					// メニューなし
	wcex.cbClsExtra		= 0;					// エキストラなし
	wcex.cbWndExtra		= 0;					
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);		// 背景色白

	if (!RegisterClassEx(&wcex)) return FALSE;	// ウインドウクラスの登録

	hwnd = CreateWindow(NAME,					// ウインドウクラスの名前
				TITLE,		 					// タイトル
				WS_CAPTION | WS_SYSMENU,		// ウインドウスタイル
				CW_USEDEFAULT, CW_USEDEFAULT,	// X座標,Y座標
				0,								// 幅
				0,								// 高さ
				HWND_DESKTOP, 					// 親ウインドウなし
				(HMENU)NULL, 					// メニューなし
				hInstance, 						// インスタンスハンドル
				(LPVOID)NULL);					// 追加引数なし

	if (!hwnd) return FALSE;

	// ダイアログ表示
	DialogBox(
		hInstance,				// アプリのインスタンスハンドル
		(LPCSTR)IDD_DIALOG1,	// リソースエディタで作成したテンプレート
		NULL,					// 親ウィンドウハンドル
		DlgProc					// ダイアログプロシージャ
		);

	// メッセージ･ループ
	while(1)
	{
		// メッセージがあるかどうかをチェック
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
       	{
			// メッセージを取得
			if(!GetMessage(&msg, NULL, 0, 0))
			{ 
				break; 
			}
			else
			{
				TranslateMessage(&msg); 		// キーボード使用を可能にする
				DispatchMessage(&msg); 			// コントロールをWindowsに戻す
			}
		}
	}

	return msg.wParam;
}

//--------------------------------------------------
// ウインドウプロシージャ
//--------------------------------------------------
LRESULT WINAPI WndProc(
	HWND hWnd, 				// ウィンドウハンドル
	UINT uMsg,				// メッセージID
	WPARAM wParam,			// 付帯情報１
	LPARAM lParam			// 付帯情報２
	)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

//--------------------------------------------------
// ダイアログプロシージャ
//--------------------------------------------------
BOOL CALLBACK DlgProc(
	HWND hDlg, 				// ウィンドウハンドル
	UINT uMsg,				// メッセージID
	WPARAM wParam,			// 付帯情報１
	LPARAM lParam			// 付帯情報２
	)
{
	int sts;
	sockaddr_in addr;

	switch(uMsg)
	{
	case WM_INITDIALOG:	// ダイアログ生成時
		g_hDlg = hDlg;

		WSADATA		wd;						// WSAStartup用
		WORD		requiredversion;		// このプログラムが要求するバージョン

		requiredversion = MAKEWORD(2, 0);

		// WinSock初期化
		sts = WSAStartup(MAKEWORD(2, 0), &wd);
		if(sts != 0)
		{
			errcom(WSAGetLastError());
			EndDialog(hDlg, 0);
			return TRUE;
		}
		// バージョンチェック
		if(wd.wVersion != requiredversion)
		{
			MessageBox(NULL,"VERSION ERROR!", "", MB_OK);
			EndDialog(hDlg, 0);
			return TRUE;
		}

		// 発言入力ボックス/送信ボタン無効化
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT2	), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK		), FALSE);

		// モード選択メッセージ
		if(MessageBox(hDlg, "サーバーモードで起動しますか？", "モード選択", MB_YESNO) == IDYES)
		{
			// 接続先入力ボックス/接続ボタン無効化
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1		), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1	), FALSE);
			// 待機中表示
			SetDlgItemText(hDlg, IDC_EDIT1, "待受中");

			//################################################################################
			// ⑥[サーバーモード]待受ソケット生成
			//################################################################################
			g_waitsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


			if(g_waitsock == INVALID_SOCKET)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// 待受アドレス設定
			addr.sin_family = AF_INET;
			addr.sin_port = htons(PORTNO);
			addr.sin_addr.s_addr = INADDR_ANY;

			// ソケットにIPアドレス、ポート番号をセットする
			sts = bind(g_waitsock, (sockaddr*)&addr, sizeof(sockaddr_in));

			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			sts = SOCKET_ERROR;

			//################################################################################
			// ⑦[サーバーモード]接続要求受付開始
			//################################################################################
			sts = listen(g_waitsock, SOMAXCONN); // 同時に接続できる最大値ー＞SOMAXCONN


			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// 待受スレッド開始
			_beginthread(AcceptProc, 0, NULL);
		}
		else
		{
			// 接続先入力ボックスにカーソルを移動
			SetFocus(GetDlgItem(g_hDlg, IDC_EDIT1));
		}
		return TRUE;

	case WM_COMMAND:	// ボタンなどのコントロールメッセージ
		switch(wParam)
		{
		case IDC_BUTTON1:	// 接続ボタン
			// 入力ボックスからIPアドレス取得
			char ip[16];
			GetDlgItemText(hDlg, IDC_EDIT1, ip, -1);

			//################################################################################
			// ①[クライアントモード]ソケット生成
			//################################################################################
			g_chatsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


			if(g_chatsock == INVALID_SOCKET)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// 接続先設定
			addr.sin_family = AF_INET;
			addr.sin_port = htons(PORTNO);
			addr.sin_addr.s_addr = inet_addr(ip);

			sts = SOCKET_ERROR;

			//################################################################################
			// ②[クライアントモード]接続
			//################################################################################
			sts = connect(g_chatsock, (sockaddr*)&addr, sizeof(addr));


			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
			}
			else
			{
				// 接続先入力ボックス/接続ボタン無効化
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT1		), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1	), FALSE);
				// 発言入力ボックス/送信ボタン有効化
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT2		), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK			), TRUE);
				// 発言入力ボックスにカーソルを移動
				SetFocus(GetDlgItem(g_hDlg, IDC_EDIT2));

				// 受信スレッド開始
				_beginthread(RecvProc, 0, NULL);
			}
			return TRUE;

		case IDOK:			// 送信ボタン(Enterキー)
			// 入力ボックスから発言取得
			char sendbuf[RECV_MAX];
			GetDlgItemText(hDlg, IDC_EDIT2, sendbuf, -1);

			if(strlen(sendbuf) > 0)
			{
				//################################################################################
				// ③[サーバー/クライアント共通]発言送信
				// 文字列の最後の'\0'までを送信する
				// (ストリーム型通信のデータ境界はプログラマが決める。今回は'\0'を境界として使用)
				//################################################################################
				sts = send(g_chatsock, sendbuf, strlen(sendbuf) + 1, 0);


				if(sts == SOCKET_ERROR)
				{
					errcom(WSAGetLastError());
				}
				else
				{
					// 入力ボックスクリア
					SetDlgItemText(hDlg, IDC_EDIT2, "");

					// ログ追加
					AddLog("ワイ", sendbuf);
				}
			}
			return TRUE;

		case IDCANCEL:		// ×ボタン(Escキー)
			EndDialog(hDlg, 0);
			return TRUE;
		}
		return FALSE;

	case WM_DESTROY:	// ウィンドウ破棄時
		// 終了処理
		if(g_waitsock != INVALID_SOCKET)
		{
			// ソケットをクローズ
			closesocket(g_waitsock);
		}
		if(g_chatsock != INVALID_SOCKET)
		{
			//################################################################################
			// ⑤[サーバー/クライアント共通]コネクションの終了
			//################################################################################
			shutdown(g_chatsock, SD_BOTH);


			// ソケットをクローズ
			closesocket(g_chatsock);
		}

		// WinSock終了
		WSACleanup();

		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------
// [サーバーモード]待受スレッド関数
//--------------------------------------------------
void AcceptProc(void* p)
{
	sockaddr_in addr;
	int len = sizeof(sockaddr_in);

	//################################################################################
	// ⑧[サーバーモード]接続要求を受けるまで待機(ブロッキング)
	//################################################################################
	g_chatsock = accept(g_waitsock, (sockaddr*)&addr, &len); // 戻り値は接続相手との通信用ソケット


	// 戻り値は通信用ソケットとなる
	if(g_chatsock == INVALID_SOCKET)
	{
		errcom(WSAGetLastError());
		PostMessage(g_hDlg, WM_QUIT, 0, 0);
	}
	else
	{
		// 発言入力ボックス/送信ボタン有効化
		EnableWindow(GetDlgItem(g_hDlg, IDC_EDIT2	), TRUE);
		EnableWindow(GetDlgItem(g_hDlg, IDOK		), TRUE);
		// 相手のIPアドレス表示
		SetDlgItemText(g_hDlg, IDC_EDIT1, inet_ntoa(addr.sin_addr));
		// 発言入力ボックスにカーソルを移動(なぜか効かない…)
		SetFocus(GetDlgItem(g_hDlg, IDC_EDIT2));

		// 受信スレッド開始
		_beginthread(RecvProc, 0, NULL);
	}

	_endthread();
}

//--------------------------------------------------
// [サーバー/クライアント共通]受信スレッド関数
//--------------------------------------------------
void RecvProc(void* p)
{
	int recv_idx = 0;

	while(1)
	{
		int sts = SOCKET_ERROR;

		//################################################################################
		// ④[サーバー/クライアント共通]データを受信するまで待機(ブロッキング)
		// データ境界の'\0'を受信するまで1バイトずつ受信
		//################################################################################
		sts = recv(g_chatsock, &g_recvbuf[recv_idx], 1, 0);


		// 戻り値は実際に受信したサイズとなる
		if(sts <= 0)
		{
			// 負の場合はエラー(0は正常切断)
			int code = WSAGetLastError();
			if(!sts || code == WSAECONNRESET)
			{
				MessageBox(g_hDlg, "コネクションが切断されました", "切断", MB_OK);
			}
			else
			{
				errcom(code);
			}
			PostMessage(g_hDlg, WM_QUIT, 0, 0);
			break;
		}
		else
		{
			if (g_recvbuf[recv_idx++] == '\0')
			{
				// ログ追加
				AddLog("ニキ", g_recvbuf);
				recv_idx = 0;
			}
		}
	}

	_endthread();
}

//--------------------------------------------------
// チャットログ追加
//--------------------------------------------------
void AddLog(
	const char* name,
	const char* text
	)
{
	int len = SendDlgItemMessage(g_hDlg, IDC_EDIT3, WM_GETTEXTLENGTH, 0, 0);
	SendDlgItemMessage(g_hDlg, IDC_EDIT3, EM_SETSEL, len, len);
	if (len)
	{
		SendDlgItemMessage(g_hDlg, IDC_EDIT3, EM_REPLACESEL, 0, (LPARAM)"\r\n");
	}
	SendDlgItemMessage(g_hDlg, IDC_EDIT3, EM_REPLACESEL, 0, (LPARAM)name);
	SendDlgItemMessage(g_hDlg, IDC_EDIT3, EM_REPLACESEL, 0, (LPARAM)" > ");
	SendDlgItemMessage(g_hDlg, IDC_EDIT3, EM_REPLACESEL, 0, (LPARAM)text);
}
