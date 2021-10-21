#include "WindowsInitialize.h"

const wchar_t WindowsInitialize::TitleName[] = L"����";//�Q�[���^�C�g��
const int WindowsInitialize::WIN_WIDTH = 1280;//�E�B���h�E�̉���
const int WindowsInitialize::WIN_HEIGHT = 720;//�E�B���h�E�̉���

//�E�B���h�E�n���h���̎擾
HWND WindowsInitialize::GetHwnd()
{
	return hwnd;
}

//�E�B���h�E�N���X�̎擾
HINSTANCE WindowsInitialize::GetInstance()
{
	return win.hInstance;
}

//�E�B���h�E�v���V�[�W��
LRESULT WindowsInitialize::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//���b�Z�[�W�ŕ���
	switch (msg)
	{
	case WM_DESTROY://�E�B���h�E���j�����ꂽ
		PostQuitMessage(0);//OS�ɑ΂��āA�A�v���̏I����`����
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);//�W���̏������s��
}

//�E�B���h�E�̐���
void WindowsInitialize::CreateWIN()
{
	//�E�B���h�E�N���X�̐ݒ�
	win.cbSize = sizeof(WNDCLASSEX);
	win.lpfnWndProc = (WNDPROC)WindowProc;//�E�B���h�E�v���V�[�W����ݒ�
	win.lpszClassName = TitleName;//�Q�[���^�C�g��
	win.hInstance = GetModuleHandle(nullptr);//�E�B���h�E�n���h��
	win.hCursor = LoadCursor(NULL, IDC_ARROW);//�J�[�\���w��

	RegisterClassEx(&win); // �E�B���h�E�N���X��OS�ɓo�^

	RECT wrc = { 0, 0, WIN_WIDTH, WIN_HEIGHT };//�E�B���h�E�T�C�Y(X���W, Y���W, �E�B���h�E�̉���, �E�B���h�E�̏c��)
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�����ŃT�C�Y�␳

	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(win.lpszClassName,	//�N���X��
		TitleName,							//�Q�[���^�C�g��
		WS_OVERLAPPEDWINDOW,				//�W���I�ȃE�B���h�E�X�^�C��
		CW_USEDEFAULT,						//�\��X���W�iOS�ɔC����j
		CW_USEDEFAULT,						//�\��Y���W�iOS�ɔC����j
		wrc.right - wrc.left,				//�E�B���h�E�̉���
		wrc.bottom - wrc.top,				//�E�B���h�E�̏c��
		nullptr,							//�e�E�B���h�E�n���h��
		nullptr,							//���j���[�n���h��
		win.hInstance,						//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);							//�I�v�V����

	ShowWindow(hwnd, SW_SHOW);//�E�B���h�E�\��
}

//���b�Z�[�W���[�v
bool WindowsInitialize::MessageLoop()
{
	MSG msg{};	// ���b�Z�[�W

	//���b�Z�[�W������H
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);//�L�[���̓��b�Z�[�W�̏���
		DispatchMessage(&msg);//�E�B���h�E�v���V�[�W���Ƀ��b�Z�[�W�𑗂�
	}

	//�I�����b�Z�[�W�������烋�[�v�𔲂���
	if (msg.message == WM_QUIT)
	{
		return true;
	}

	return false;
}

//�E�B���h�E�̔j��
void WindowsInitialize::DeleteWIN()
{
	UnregisterClass(win.lpszClassName, win.hInstance);//�E�B���h�E�N���X��o�^����
}