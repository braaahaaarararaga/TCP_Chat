//--------------------------------------------------
//
//		TCP 1on1�`���b�g�v���O����
//
//--------------------------------------------------
#include <winsock2.h>
#include <process.h>
#include "wsock32error.h"
#include "resource.h"

//--------------------------------------------------
// �}�N��
//--------------------------------------------------
#define PORTNO		(20250)
#define NAME		"MainWindow"
#define TITLE		"TCP 1on1�`���b�g"
#define RECV_MAX	(1024)

//--------------------------------------------------
// ���C�u����
//--------------------------------------------------
#pragma	comment(lib,"ws2_32.lib")
#pragma	comment(lib,"winmm.lib")

//--------------------------------------------------
// �v���g�^�C�v
//--------------------------------------------------
LRESULT CALLBACK WndProc(	// �E�B���h�E�v���V�[�W��
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	);

BOOL CALLBACK DlgProc(		// �_�C�A���O�v���V�[�W��
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	);

void AcceptProc(void* p);	// �Ҏ�X���b�h�֐�
void RecvProc(void* p);		// ��M�X���b�h�֐�
void AddLog(				// �`���b�g���O�ǉ�
	const char* name,
	const char* text
	);

//--------------------------------------------------
// �O���[�o���ϐ�
//--------------------------------------------------
SOCKET g_waitsock = INVALID_SOCKET;	// �Ҏ�p�\�P�b�g(�T�[�o�[�̂ݎg�p)
SOCKET g_chatsock = INVALID_SOCKET;	// �ʐM�p�\�P�b�g

HWND g_hDlg;				// �_�C�A���O�E�B���h�E�n���h��
char g_recvbuf[RECV_MAX];	// ��M�o�b�t�@

//--------------------------------------------------
// WinMain�֐�
//--------------------------------------------------
int APIENTRY WinMain(
	HINSTANCE 	hInstance, 		// �A�v���P�[�V�����C���X�^���X�l
	HINSTANCE 	hPrevInstance,	// �Ӗ��Ȃ�
	LPSTR 		lpszArgs, 		// �N�����̈���������
	int 		nWinMode		// �E�C���h�E�\�����[�h
	)
{
	HWND			hwnd;						// �E�C���h�E�n���h��
	MSG				msg;						// ���b�Z�[�W�\����
	WNDCLASSEX		wcex;						// �E�C���h�E�N���X�\����

	// �E�C���h�E�N���X���̃Z�b�g
	wcex.hInstance		= hInstance;			// �C���X�^���X�l�̃Z�b�g
	wcex.lpszClassName	= NAME;					// �N���X��
	wcex.lpfnWndProc	= (WNDPROC)WndProc;		// �E�C���h�E���b�Z�[�W�֐�
	wcex.style			= 0;					// �E�C���h�E�X�^�C��
	wcex.cbSize 		= sizeof(WNDCLASSEX);	// �\���̂̃T�C�Y
	wcex.hIcon			= LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);	// ���[�W�A�C�R��
	wcex.hIconSm		= LoadIcon((HINSTANCE)NULL, IDI_WINLOGO);		// �X���[���A�C�R��
	wcex.hCursor		= LoadCursor((HINSTANCE)NULL, IDC_ARROW);		// �J�[�\���X�^�C��
	wcex.lpszMenuName	= 0; 					// ���j���[�Ȃ�
	wcex.cbClsExtra		= 0;					// �G�L�X�g���Ȃ�
	wcex.cbWndExtra		= 0;					
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);		// �w�i�F��

	if (!RegisterClassEx(&wcex)) return FALSE;	// �E�C���h�E�N���X�̓o�^

	hwnd = CreateWindow(NAME,					// �E�C���h�E�N���X�̖��O
				TITLE,		 					// �^�C�g��
				WS_CAPTION | WS_SYSMENU,		// �E�C���h�E�X�^�C��
				CW_USEDEFAULT, CW_USEDEFAULT,	// X���W,Y���W
				0,								// ��
				0,								// ����
				HWND_DESKTOP, 					// �e�E�C���h�E�Ȃ�
				(HMENU)NULL, 					// ���j���[�Ȃ�
				hInstance, 						// �C���X�^���X�n���h��
				(LPVOID)NULL);					// �ǉ������Ȃ�

	if (!hwnd) return FALSE;

	// �_�C�A���O�\��
	DialogBox(
		hInstance,				// �A�v���̃C���X�^���X�n���h��
		(LPCSTR)IDD_DIALOG1,	// ���\�[�X�G�f�B�^�ō쐬�����e���v���[�g
		NULL,					// �e�E�B���h�E�n���h��
		DlgProc					// �_�C�A���O�v���V�[�W��
		);

	// ���b�Z�[�W����[�v
	while(1)
	{
		// ���b�Z�[�W�����邩�ǂ������`�F�b�N
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
       	{
			// ���b�Z�[�W���擾
			if(!GetMessage(&msg, NULL, 0, 0))
			{ 
				break; 
			}
			else
			{
				TranslateMessage(&msg); 		// �L�[�{�[�h�g�p���\�ɂ���
				DispatchMessage(&msg); 			// �R���g���[����Windows�ɖ߂�
			}
		}
	}

	return msg.wParam;
}

//--------------------------------------------------
// �E�C���h�E�v���V�[�W��
//--------------------------------------------------
LRESULT WINAPI WndProc(
	HWND hWnd, 				// �E�B���h�E�n���h��
	UINT uMsg,				// ���b�Z�[�WID
	WPARAM wParam,			// �t�я��P
	LPARAM lParam			// �t�я��Q
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
// �_�C�A���O�v���V�[�W��
//--------------------------------------------------
BOOL CALLBACK DlgProc(
	HWND hDlg, 				// �E�B���h�E�n���h��
	UINT uMsg,				// ���b�Z�[�WID
	WPARAM wParam,			// �t�я��P
	LPARAM lParam			// �t�я��Q
	)
{
	int sts;
	sockaddr_in addr;

	switch(uMsg)
	{
	case WM_INITDIALOG:	// �_�C�A���O������
		g_hDlg = hDlg;

		WSADATA		wd;						// WSAStartup�p
		WORD		requiredversion;		// ���̃v���O�������v������o�[�W����

		requiredversion = MAKEWORD(2, 0);

		// WinSock������
		sts = WSAStartup(MAKEWORD(2, 0), &wd);
		if(sts != 0)
		{
			errcom(WSAGetLastError());
			EndDialog(hDlg, 0);
			return TRUE;
		}
		// �o�[�W�����`�F�b�N
		if(wd.wVersion != requiredversion)
		{
			MessageBox(NULL,"VERSION ERROR!", "", MB_OK);
			EndDialog(hDlg, 0);
			return TRUE;
		}

		// �������̓{�b�N�X/���M�{�^��������
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT2	), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK		), FALSE);

		// ���[�h�I�����b�Z�[�W
		if(MessageBox(hDlg, "�T�[�o�[���[�h�ŋN�����܂����H", "���[�h�I��", MB_YESNO) == IDYES)
		{
			// �ڑ�����̓{�b�N�X/�ڑ��{�^��������
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1		), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1	), FALSE);
			// �ҋ@���\��
			SetDlgItemText(hDlg, IDC_EDIT1, "�Ҏ�");

			//################################################################################
			// �E[�T�[�o�[���[�h]�Ҏ�\�P�b�g����
			//################################################################################
			g_waitsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


			if(g_waitsock == INVALID_SOCKET)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// �Ҏ�A�h���X�ݒ�
			addr.sin_family = AF_INET;
			addr.sin_port = htons(PORTNO);
			addr.sin_addr.s_addr = INADDR_ANY;

			// �\�P�b�g��IP�A�h���X�A�|�[�g�ԍ����Z�b�g����
			sts = bind(g_waitsock, (sockaddr*)&addr, sizeof(sockaddr_in));

			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			sts = SOCKET_ERROR;

			//################################################################################
			// �F[�T�[�o�[���[�h]�ڑ��v����t�J�n
			//################################################################################
			sts = listen(g_waitsock, SOMAXCONN); // �����ɐڑ��ł���ő�l�[��SOMAXCONN


			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// �Ҏ�X���b�h�J�n
			_beginthread(AcceptProc, 0, NULL);
		}
		else
		{
			// �ڑ�����̓{�b�N�X�ɃJ�[�\�����ړ�
			SetFocus(GetDlgItem(g_hDlg, IDC_EDIT1));
		}
		return TRUE;

	case WM_COMMAND:	// �{�^���Ȃǂ̃R���g���[�����b�Z�[�W
		switch(wParam)
		{
		case IDC_BUTTON1:	// �ڑ��{�^��
			// ���̓{�b�N�X����IP�A�h���X�擾
			char ip[16];
			GetDlgItemText(hDlg, IDC_EDIT1, ip, -1);

			//################################################################################
			// �@[�N���C�A���g���[�h]�\�P�b�g����
			//################################################################################
			g_chatsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


			if(g_chatsock == INVALID_SOCKET)
			{
				errcom(WSAGetLastError());
				EndDialog(hDlg, 0);
				return TRUE;
			}

			// �ڑ���ݒ�
			addr.sin_family = AF_INET;
			addr.sin_port = htons(PORTNO);
			addr.sin_addr.s_addr = inet_addr(ip);

			sts = SOCKET_ERROR;

			//################################################################################
			// �A[�N���C�A���g���[�h]�ڑ�
			//################################################################################
			sts = connect(g_chatsock, (sockaddr*)&addr, sizeof(addr));


			if(sts == SOCKET_ERROR)
			{
				errcom(WSAGetLastError());
			}
			else
			{
				// �ڑ�����̓{�b�N�X/�ڑ��{�^��������
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT1		), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1	), FALSE);
				// �������̓{�b�N�X/���M�{�^���L����
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT2		), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK			), TRUE);
				// �������̓{�b�N�X�ɃJ�[�\�����ړ�
				SetFocus(GetDlgItem(g_hDlg, IDC_EDIT2));

				// ��M�X���b�h�J�n
				_beginthread(RecvProc, 0, NULL);
			}
			return TRUE;

		case IDOK:			// ���M�{�^��(Enter�L�[)
			// ���̓{�b�N�X���甭���擾
			char sendbuf[RECV_MAX];
			GetDlgItemText(hDlg, IDC_EDIT2, sendbuf, -1);

			if(strlen(sendbuf) > 0)
			{
				//################################################################################
				// �B[�T�[�o�[/�N���C�A���g����]�������M
				// ������̍Ō��'\0'�܂ł𑗐M����
				// (�X�g���[���^�ʐM�̃f�[�^���E�̓v���O���}�����߂�B�����'\0'�����E�Ƃ��Ďg�p)
				//################################################################################
				sts = send(g_chatsock, sendbuf, strlen(sendbuf) + 1, 0);


				if(sts == SOCKET_ERROR)
				{
					errcom(WSAGetLastError());
				}
				else
				{
					// ���̓{�b�N�X�N���A
					SetDlgItemText(hDlg, IDC_EDIT2, "");

					// ���O�ǉ�
					AddLog("���C", sendbuf);
				}
			}
			return TRUE;

		case IDCANCEL:		// �~�{�^��(Esc�L�[)
			EndDialog(hDlg, 0);
			return TRUE;
		}
		return FALSE;

	case WM_DESTROY:	// �E�B���h�E�j����
		// �I������
		if(g_waitsock != INVALID_SOCKET)
		{
			// �\�P�b�g���N���[�Y
			closesocket(g_waitsock);
		}
		if(g_chatsock != INVALID_SOCKET)
		{
			//################################################################################
			// �D[�T�[�o�[/�N���C�A���g����]�R�l�N�V�����̏I��
			//################################################################################
			shutdown(g_chatsock, SD_BOTH);


			// �\�P�b�g���N���[�Y
			closesocket(g_chatsock);
		}

		// WinSock�I��
		WSACleanup();

		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------
// [�T�[�o�[���[�h]�Ҏ�X���b�h�֐�
//--------------------------------------------------
void AcceptProc(void* p)
{
	sockaddr_in addr;
	int len = sizeof(sockaddr_in);

	//################################################################################
	// �G[�T�[�o�[���[�h]�ڑ��v�����󂯂�܂őҋ@(�u���b�L���O)
	//################################################################################
	g_chatsock = accept(g_waitsock, (sockaddr*)&addr, &len); // �߂�l�͐ڑ�����Ƃ̒ʐM�p�\�P�b�g


	// �߂�l�͒ʐM�p�\�P�b�g�ƂȂ�
	if(g_chatsock == INVALID_SOCKET)
	{
		errcom(WSAGetLastError());
		PostMessage(g_hDlg, WM_QUIT, 0, 0);
	}
	else
	{
		// �������̓{�b�N�X/���M�{�^���L����
		EnableWindow(GetDlgItem(g_hDlg, IDC_EDIT2	), TRUE);
		EnableWindow(GetDlgItem(g_hDlg, IDOK		), TRUE);
		// �����IP�A�h���X�\��
		SetDlgItemText(g_hDlg, IDC_EDIT1, inet_ntoa(addr.sin_addr));
		// �������̓{�b�N�X�ɃJ�[�\�����ړ�(�Ȃ��������Ȃ��c)
		SetFocus(GetDlgItem(g_hDlg, IDC_EDIT2));

		// ��M�X���b�h�J�n
		_beginthread(RecvProc, 0, NULL);
	}

	_endthread();
}

//--------------------------------------------------
// [�T�[�o�[/�N���C�A���g����]��M�X���b�h�֐�
//--------------------------------------------------
void RecvProc(void* p)
{
	int recv_idx = 0;

	while(1)
	{
		int sts = SOCKET_ERROR;

		//################################################################################
		// �C[�T�[�o�[/�N���C�A���g����]�f�[�^����M����܂őҋ@(�u���b�L���O)
		// �f�[�^���E��'\0'����M����܂�1�o�C�g����M
		//################################################################################
		sts = recv(g_chatsock, &g_recvbuf[recv_idx], 1, 0);


		// �߂�l�͎��ۂɎ�M�����T�C�Y�ƂȂ�
		if(sts <= 0)
		{
			// ���̏ꍇ�̓G���[(0�͐���ؒf)
			int code = WSAGetLastError();
			if(!sts || code == WSAECONNRESET)
			{
				MessageBox(g_hDlg, "�R�l�N�V�������ؒf����܂���", "�ؒf", MB_OK);
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
				// ���O�ǉ�
				AddLog("�j�L", g_recvbuf);
				recv_idx = 0;
			}
		}
	}

	_endthread();
}

//--------------------------------------------------
// �`���b�g���O�ǉ�
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
