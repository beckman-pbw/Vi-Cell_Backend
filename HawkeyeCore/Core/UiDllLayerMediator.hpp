#pragma once

#include <functional>
#include <vector>
#include <type_traits>
#include <future>

#include "HawkeyeError.hpp"
#include "HawkeyeThreadPool.hpp"

template <typename T>
struct has_member_tocstyle
{
private:
	template <class, class> class checkMember;

	template<typename C>
	static std::true_type check(checkMember<C, decltype(&C::ToCStyle)> *);

	template<typename C>
	static std::false_type check(...);

public:
	typedef decltype(check<T>(nullptr)) type;
	static const bool value = std::is_same<std::true_type, decltype(check<T>(nullptr))>::value;
};

template<typename T>
struct is_vector : std::false_type {};

template<typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type {};

class UiDllLayerMediator final
{
public:
	UiDllLayerMediator(std::shared_ptr<boost::asio::io_context> pMainIoSvc);
	~UiDllLayerMediator();

	template<typename RTYPE>
	RTYPE entryPoint(std::function<RTYPE()> workLambda, const std::string& callerName, bool enableLog = false)
	{
		assert(workLambda);

		if (!pMainIoSvc_)
		{
			return { };
		}

		printLog("<Enter : entryPoint " + callerName, enableLog);
		
		auto promise = std::promise<RTYPE>();
		pMainIoSvc_->post([workLambda, &promise]() -> void
		{
			RTYPE returnValue = workLambda();
			promise.set_value(returnValue);
		});

		auto status = promise.get_future().get();

		printLog("<Exit : entryPoint " + callerName, enableLog);

		return status;
	}
	
	template<>
	void entryPoint(std::function<void()> workLambda, const std::string& callerName, bool enableLog)
	{
		assert(workLambda);

		if (!pMainIoSvc_)
		{
			return;
		}

		printLog("<Enter : entryPoint<void> " + callerName, enableLog);

		auto promise = std::promise<void>();
		
		pMainIoSvc_->post([workLambda, &promise]() -> void
		{			
			workLambda();
			promise.set_value();
		});

		auto future = promise.get_future();
		future.get();

		printLog("<Exit : entryPoint<void> " + callerName, enableLog);
	}

	template<typename RTYPE>
	RTYPE entryPoint(std::function<void(std::function<void(RTYPE)>)> workLambda, const std::string& callerName, bool enableLog = false)
	{
		assert(workLambda);

		printLog("<Enter : entryPoint<async> " + callerName, enableLog);

		auto promise = std::promise<RTYPE>();
		pMainIoSvc_->post([workLambda, &promise]() -> void
		{
			auto oncomplete = [&promise](RTYPE he)
			{
				promise.set_value(he);
			};
			workLambda(oncomplete);
		});

		auto status = promise.get_future().get();
		
		printLog("<Exit : entryPoint<async> " + callerName, enableLog);

		return status;
	}

	template<typename ... Args>
	auto wrapCallback(void(*callback)(Args...))
	{
		assert(callback);

		return [=](auto... callback_args) -> void
		{
			runCallbackOnUniqueThread([=]()
			{
				callback(GetCType(callback_args)...);
			});
		};
	}

	template<typename ... Args>
	auto wrapCallbackWithDeleter(void(*callback)(Args...), std::function<void(Args...)> deleterCallback)
	{
		assert(callback);
		assert(deleterCallback);

		auto work = [=](auto... callback_args) -> void
		{
			callback(callback_args...);
			deleterCallback(callback_args...);
		};

		return [=](auto... callback_args) -> void
		{
			runCallbackOnUniqueThread([=]()
			{
				work(GetCType(callback_args)...);
			});
		};
	}

private:

	template <typename T>
	auto GetCType(std::vector<T> val) -> decltype(std::declval<T>().ToCStyle())*
	{
		typedef decltype(std::declval<T>().ToCStyle()) returnType;

		size_t numObj = 0;
		returnType* obj = nullptr;

		DataConversion::convert_vecOfDllType_to_listOfCType<T, returnType, size_t>
			(val, obj, numObj);

		return obj;
	}

	template <typename T>
	typename std::enable_if<!is_vector<T>::value && has_member_tocstyle<T>::value, decltype(std::declval<T>().ToCStyle())>::type*
		GetCType(T val)
	{
		typedef decltype(std::declval<T>().ToCStyle()) returnType;

		returnType* obj = nullptr;
		DataConversion::convert_dllType_to_cType<T, returnType>(val, obj);
		
		return obj;
	}

	template <typename T>
	typename std::enable_if<!is_vector<T>::value, decltype(std::declval<T>().ToCStyle())>::type*
		GetCType(std::shared_ptr<T> val)
	{
		if (!val)
		{
			return nullptr;
		}
		return GetCType(*val);
	}

	template <typename T>
	typename std::enable_if<!is_vector<T>::value && !has_member_tocstyle<T>::value, T>::type
		GetCType(T val)
	{
		return val;
	}

	void runCallbackOnUniqueThread(std::function<void()> work);
	void printLog(std::string& logInfo, bool enableLog);

	std::shared_ptr<boost::asio::io_context> pMainIoSvc_;
	std::unique_ptr<HawkeyeThreadPool> pThreadPool_;
};
