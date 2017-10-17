#pragma once
#define WIN32_LEAN_AND_MEAN

#include "SccMediator.h"

#ifdef SccTcpMediator_EXPORTS
#define SccTcpMediator_API __declspec(dllexport)
#else
#define SccTcpMediator_API __declspec(dllimport)
#endif

#pragma comment(lib, "advapi32.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

namespace SccTcpMediator
{
	extern LPSTMDATA sccTcpMediator;

	extern "C++" {
		// Список серийный номеров подключенных устройств
		SccTcpMediator_API  std::vector<UINT> GetDeviceNums();
		// Отправить список слов Arinc429 в формате структуры Arinc429 (время, данные)
		SccTcpMediator_API  INT Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> &words);
		// Получить список принятых слов Arinc429 в формате структуры Arinc429 (время, данные)
		SccTcpMediator_API  INT Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words);
		// Отправить список слов Arinc708 в формате структуры Arinc708 (время, данные(200 байт))
		SccTcpMediator_API  INT Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> &words);
		// Получить список принятых слов Arinc708 в формате структуры Arinc708 (время, данные(200 байт))
		SccTcpMediator_API  INT Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words);
	}
	extern "C" {
		// InitializeEnvironment SCC TCP Mediator 
		// Return ZERO on succes error code on failure
		SccTcpMediator_API INT STMStartup(LPTSTR hostNameTCP, USHORT port, LPWSTR hostNameUDP);
		// InitializeEnvironment SCC TCP Mediator with default parameters for loopback address
		// Return ZERO on succes error code on failure
		SccTcpMediator_API INT STMStartupLocal();
		// Return time in microseconds needed to process simple command via TCP
		SccTcpMediator_API INT Ping();
		// Close all UDP/TCP streams connected with this thread
		SccTcpMediator_API void Disconnect();
		// Return status of connection: true if connections established with server, false - if no connections
		SccTcpMediator_API bool IsOnline();

		// Return INT, bits in one mean is channel avaliable or not
		// 1 - is avaliable, 0 - is aready on use
		// first 4 bits bits are 1-4 numbers of 429 channels
		// 5th bit is 708 channel
		// If the function fails, the return value is negative error code
		// If the function succeeds, the return value is (0-31)
		SccTcpMediator_API INT GetAvalibleChannelsIn(UINT deviceSerialNum);
		SccTcpMediator_API INT GetAvalibleChannelsOut(UINT deviceSerialNum);

		// Return TRUE on success, error code on failure
		SccTcpMediator_API INT ReleaseChannel429In(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ReleaseChannel429Out(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ReleaseChannel708In(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ReleaseChannel708Out(UINT deviceSerialNum, UINT channelNum);

		// Return TRUE on success, error code on failure
		SccTcpMediator_API INT ConnectForced429In(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ConnectForced429Out(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ConnectForced708In(UINT deviceSerialNum, UINT channelNum);
		SccTcpMediator_API INT ConnectForced708Out(UINT deviceSerialNum, UINT channelNum);

#pragma region SccLib Functions
		// Количество подключенных устройств
		SccTcpMediator_API INT GetDeviceCount();
		/**
		* \brief Установить параметры входного канала Arinc429
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \param rate Скорость
		* \param parityType Четность
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType = Arinc429ParityTypeIn::Analys);
		/**
		* \brief Установить параметры выходного канала Arinc429
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \param rate Скорость
		* \param parityType Четность
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType = Arinc429ParityTypeOut::Odd);
		/**
		* \brief Количество неотправленных слов в выходном канале Arinc429
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API UINT Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Количество неотправленных слов в выходном канале Arinc708
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API UINT Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Суммарное время неотправленных слов в выходном канале Arinc429 в мкс
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API UINT Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Суммарное время неотправленных слов в выходном канале Arinc708 в мкс
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API UINT Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Установить максимальное количество слов во входном буфере канала Arinc429 (старые слова будут удаляться)
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \param wordsCount количество слов в штуках (Word429)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
		/**
		* \brief Установить максимальное количество слов во входном буфере канала Arinc708 (старые слова будут удаляться)
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \param wordsCount количество слов в штуках (Word708)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
		/**
		* \brief Очистка выходного буфера канала Arinc429
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT ResetOut429Channel(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Очистка выходного буфера канала Arinc708
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT ResetOut708Channel(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Очистка входного буфера канала Arinc429
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT ResetIn429Channel(UINT deviceSerialNum, UINT channelNum);
		/**
		* \brief Очистка входного буфера канала Arinc708
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is zero.
		*/
		SccTcpMediator_API INT ResetIn708Channel(UINT deviceSerialNum, UINT channelNum);
		// Устанавливает функцию для обработки события при появлении нового устройства
		SccTcpMediator_API void SetDeviceAttachedHandler(DeviceListChanged handler);
		// Устанавливает функцию для обработки события при пропадании устройства
		SccTcpMediator_API void SetDeviceDetachedHandler(DeviceListChanged handler);
		/**
		* \brief Список серийный номеров подключенных устройств
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API INT GetDeviceNumsRaw(UINT* nums, UINT max_size);
		/**
		* \brief Отправить список слов Arinc429 в формате структуры Word429 (время, данные)
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \param words Список слов
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API INT Send429WordsRaw(UINT deviceSerialNum, UINT channelNum, const Word429* words, UINT count);
		/**
		* \brief Получить список принятых слов Arinc429 в формате структуры Word429 (время, данные)
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (0 - 3)
		* \param words Список слов
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API INT Receive429WordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, UINT count);
		/**
		* \brief Отправить список слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \param words Список слов
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value isnon-negative value.
		*/
		SccTcpMediator_API INT Send708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT count);
		/**
		* \brief Получить список принятых слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
		* \param deviceSerialNum Серийный номер устройства
		* \param channelNum Номер канала в устройстве (всегда 0)
		* \param words Список слов
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is non-negative value.
		*/
		SccTcpMediator_API INT Receive708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT max_count);
		/**
		* \brief Получить конфигурацию разъема
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is pin configuration value value.
		*/
		SccTcpMediator_API INT GetPinConfiguration(UINT deviceSerialNum);
		/**
		* \brief Получить версию библиотеки ППК
		* \return If the function fails, the return value is negative error code.
		* \return If the function succeeds, the return value is dll version positive value.
		*/
		SccTcpMediator_API INT GetDllVersion();
#pragma endregion 
	}
}
