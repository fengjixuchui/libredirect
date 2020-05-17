#define INITGUID
#include <cassert>

#include "libredirect.h"


DWORD libredirect_init(DWORD init_type, HANDLE* engine_handle)
{
	FWPM_SESSION session = { 0 };
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;	// session结束后自动销毁所有callout和filter
	auto status = FwpmEngineOpen(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session, engine_handle);
	if (status != ERROR_SUCCESS)
	{
		return status;
	}

	status = FwpmTransactionBegin(*engine_handle, 0);
	if (status != ERROR_SUCCESS)
	{
		return status;
	}


	if (init_type & LIBREDIRECT_INIT_IPV4)
	{
		FWPM_CALLOUT callout_v4 = { 0 };
		FWPM_DISPLAY_DATA display_data = { 0 };
		wchar_t callout_display_name_v4[] = L"LibredirectCalloutV4";
		wchar_t callout_display_desc_v4[] = L"IPv4 callout for Libredirect";
		display_data.name = callout_display_name_v4;
		display_data.description = callout_display_desc_v4;

		callout_v4.calloutKey = LIBREDIRECT_CALLOUT_GUID_V4;
		callout_v4.displayData = display_data;
		callout_v4.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
		callout_v4.flags = 0;
		status = FwpmCalloutAdd(*engine_handle, &callout_v4, nullptr, nullptr);
		if (status != ERROR_SUCCESS)
		{
			FwpmTransactionAbort(*engine_handle);
			return status;
		}
	}
	if (init_type & LIBREDIRECT_INIT_IPV6)
	{
		FWPM_CALLOUT callout_v6 = { 0 };
		FWPM_DISPLAY_DATA display_data_v6 = { 0 };
		wchar_t callout_display_name_v6[] = L"LibredirectCalloutV6";
		wchar_t callout_display_desc_v6[] = L"IPv6 callout for Libredirect";
		display_data_v6.name = callout_display_name_v6;
		display_data_v6.description = callout_display_desc_v6;

		callout_v6.calloutKey = LIBREDIRECT_CALLOUT_GUID_V6;
		callout_v6.displayData = display_data_v6;
		callout_v6.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
		callout_v6.flags = 0;
		status = FwpmCalloutAdd(*engine_handle, &callout_v6, nullptr, nullptr);
		if (status != ERROR_SUCCESS)
		{
			FwpmTransactionAbort(*engine_handle);
			return status;
		}
	}



	FWPM_SUBLAYER sublayer = { 0 };
	sublayer.subLayerKey = LIBREDIRECT_SUBLAYER_GUID;
	wchar_t sublayer_display_name[] = L"LibredirectSublayer";;
	sublayer.displayData.name = sublayer_display_name;
	wchar_t sublayer_display_desc[] = L"Sublayer for Libredirect";
	sublayer.displayData.description = sublayer_display_desc;
	sublayer.flags = 0;
	sublayer.weight = 0x0f;
	status = FwpmSubLayerAdd(*engine_handle, &sublayer, nullptr);
	if (status != ERROR_SUCCESS)
	{
		FwpmTransactionAbort(*engine_handle);
		return status;
	}

	if (init_type & LIBREDIRECT_INIT_IPV4)
	{
		FWPM_FILTER filter_v4 = { 0 };
		UINT64 filter_id_v4 = 0;
		wchar_t filter_display_name_v4[] = L"LibredirectFilterV4";
		filter_v4.displayData.name = filter_display_name_v4;
		wchar_t filter_display_desc_v4[] = L"IPv4 filter for Libredirect";
		filter_v4.displayData.description = filter_display_desc_v4;
		filter_v4.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter_v4.subLayerKey = LIBREDIRECT_SUBLAYER_GUID;
		filter_v4.weight.type = FWP_UINT8;
		filter_v4.weight.uint8 = 0xf;
		filter_v4.numFilterConditions = 0;
		filter_v4.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
		filter_v4.action.calloutKey = LIBREDIRECT_CALLOUT_GUID_V4;
		status = FwpmFilterAdd(*engine_handle, &filter_v4, nullptr, &filter_id_v4);
		if (status != ERROR_SUCCESS)
		{
			FwpmTransactionAbort(*engine_handle);
			return status;
		}
	}

	if (init_type & LIBREDIRECT_INIT_IPV6)
	{
		FWPM_FILTER filter_v6 = { 0 };
		UINT64 filter_id_v6 = 0;
		wchar_t filter_display_name_v6[] = L"LibredirectFilterV6";
		filter_v6.displayData.name = filter_display_name_v6;
		wchar_t filter_display_desc_v6[] = L"IPv6 filter for Libredirect";
		filter_v6.displayData.description = filter_display_desc_v6;
		filter_v6.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter_v6.subLayerKey = LIBREDIRECT_SUBLAYER_GUID;
		filter_v6.weight.type = FWP_UINT8;
		filter_v6.weight.uint8 = 0xf;
		filter_v6.numFilterConditions = 0;
		filter_v6.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
		filter_v6.action.calloutKey = LIBREDIRECT_CALLOUT_GUID_V6;
		status = FwpmFilterAdd(*engine_handle, &filter_v6, nullptr, &filter_id_v6);
		if (status != ERROR_SUCCESS)
		{
			FwpmTransactionAbort(*engine_handle);
			return status;
		}
	}

	status = FwpmTransactionCommit(*engine_handle);

	return status;
}

void libredirect_uninit(HANDLE engine_handle)
{
	//关闭引擎的handle会自动删除filter，转发引擎也会停止工作
	FwpmEngineClose(engine_handle);
}

HANDLE libredirect_open()
{
	return CreateFileA("\\\\.\\libredirect",
		GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
}

void libredirect_close(HANDLE handle)
{
	CloseHandle(handle);
}



int libredirect_read_connect(HANDLE handle, connect_t* conn)
{
	DWORD recv_num;
	auto ret = DeviceIoControl(handle, IOCTL_GET_CONN, nullptr, 0, conn, sizeof(connect_t), &recv_num, nullptr);
	if (!ret || recv_num != sizeof(connect_t))
	{
		return 0;
	}
	assert(conn->ip_version == 4 || conn->ip_version == 6);

	return 1;
}

int libredirect_write_connect(HANDLE handle, connect_t* conn)
{
	DWORD recv_num;
	return DeviceIoControl(handle, IOCTL_SET_CONN, conn, sizeof(connect_t), nullptr, 0, &recv_num, nullptr);
}

