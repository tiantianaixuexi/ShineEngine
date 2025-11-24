//module;
//
//
//namespace shine::compile
//{
//	template <typename T>
//	concept builder_meta = requires {
//		typename T::builder_t;
//		typename T::interface_t;
//		{ T::uninitialized() } -> std::same_as<typename T::interface_t>;
//	};
//
//	template <builder_meta T> using builder_t = typename T::builder_t;
//	template <builder_meta T> using interface_t = typename T::interface_t;
//
//	namespace match {
//	
//		//瑕佹眰绫诲瀷 T锛堝幓闄?const/volatile 鍜屽紩鐢ㄤ慨楗板悗锛夊繀椤绘湁涓€涓祵濂楃被鍨?is_matcher
//		template<typename T>
//		concept matcher = requires { typename std::remove_cvref_t<T>::is_matcher; };
//
//		//	棣栧厛瑕佹眰 T 婊¤冻涓婇潰瀹氫箟鐨?matcher 姒傚康
//		//  鍏舵锛岃姹?T 鐨勫父閲忓紩鐢?t 鍜?Event 鐨勫父閲忓紩鐢?e 婊¤冻浠ヤ笅鏉′欢锛?
//		//		t(e) 蹇呴』鏄彲浠ヨ浆鎹负 bool 鐨勮〃杈惧紡锛堝嵆 T 蹇呴』鑳藉儚鍑芥暟涓€鏍风敤 Event 璋冪敤锛屽苟杩斿洖甯冨皵鍊硷級銆?
//		//		t 蹇呴』鏈?describe() 鎴愬憳鍑芥暟銆?
//		//		t 蹇呴』鏈?describe_match(e) 鎴愬憳鍑芥暟銆?
//		template<typename T,typename Event>
//		concept matcher_for = matcher<T> and requires (T const& t,Event const& e){	
//			{ t(e) }->std::convertible_to<bool>;
//			t.describe();
//			t.describe_match(e);
//		};
//
//	};
//
//
//
//	namespace detail {
//		template <typename T> struct ct_helper {
//			consteval ct_helper(T t) : value(t) {}
//			T value;
//		};
//		template <typename T> ct_helper(T) -> ct_helper<T>;
//
//
//	} // namespace detail
//
//	template <detail::ct_helper Value> consteval auto ct() {
//		return std::integral_constant<decltype(Value.value), Value.value>{};
//	}
//
//	template <typename T> consteval auto ct() { return std::type_identity<T>{}; }
//
//
//	template <std::size_t N> struct ct_string;
//
//	namespace detail {
//		template <typename T>
//		concept format_convertible = requires(T t) {
//			{ T::ct_string_convertible() } -> std::same_as<std::true_type>;
//			{ ct_string{ t.str.value } };
//		};
//	} 
//
//	template <std::size_t N > struct ct_string
//	{
//	
//		consteval ct_string() = default;
//
//
//		//explicit(false)锛氭瀯閫犲嚱鏁版棦鍙互鐢ㄤ簬鏄惧紡鏋勯€狅紝涔熷厑璁稿彂鐢熼殣寮忕被鍨嬭浆鎹?
//		consteval explicit(false) ct_string(char const(&str)[N]){
//			for (auto i = std::size_t{}; i < N; ++i) {
//				value[i] = str[i];
//			}
//		}
//
//		template <detail::format_convertible T>
//		consteval explicit(false) ct_string(T t) : ct_string(t.str.value) {}
//
//
//		consteval explicit(true) ct_string(char const* str, std::size_t sz) {
//			for (auto i = std::size_t{}; i < sz; ++i) {
//				value[i] = str[i];
//			}
//		}
//
//		consteval explicit(true) ct_string(std::string_view str)
//			:ct_string(str.data(),str.size()){}
//
//		[[nodiscard]] constexpr auto begin()  { return value.begin(); }
//		[[nodiscard]] constexpr auto end()  { return value.end() - 1; }
//		[[nodiscard]] constexpr auto begin() const  {
//			return value.begin();
//		}
//		[[nodiscard]] constexpr auto end() const  {
//			return value.end() - 1;
//		}
//		[[nodiscard]] constexpr auto rbegin() const  {
//			return ++value.rbegin();
//		}
//		[[nodiscard]] constexpr auto rend() const  {
//			return value.rend();
//		}
//
//		constexpr static std::integral_constant<std::size_t, N> capacity{};
//		constexpr static std::integral_constant<std::size_t, N - 1U> size{};
//		constexpr static std::integral_constant<bool, N == 1U> empty{};
//
//		constexpr explicit(true) operator std::string_view() const {
//			return std::string_view{ value.data(), size() };
//		}
//
//		std::array<char,N> value;
//	};
//
//	template <detail::format_convertible T>
//	ct_string(T) -> ct_string<decltype(std::declval<T>().str.value)::capacity()>;
//
//	template <std::size_t N, std::size_t M>
//	[[nodiscard]] constexpr auto operator==(ct_string<N> const& lhs,
//		ct_string<M> const& rhs) -> bool {
//		return static_cast<std::string_view>(lhs) ==
//			static_cast<std::string_view>(rhs);
//	}
//
//	template <template <typename C, C...> typename T, char... Cs>
//	[[nodiscard]] consteval auto ct_string_from_type(T<char, Cs...>) {
//		return ct_string<sizeof...(Cs) + 1U>{{Cs..., 0}};
//	}
//
//	template <ct_string S, template <typename C, C...> typename T>
//	[[nodiscard]] consteval auto ct_string_to_type() {
//		return[&]<auto... Is>(std::index_sequence<Is...>) {
//			return T<char, std::get<Is>(S.value)...>{};
//		}(std::make_index_sequence<S.size()>{});
//	}
//
//	template <ct_string S, template <typename C, C...> typename T>
//	using ct_string_to_type_t = decltype(ct_string_to_type<S, T>());
//
//	template <ct_string S, char C> [[nodiscard]] consteval auto split() {
//		constexpr auto it = [] {
//			for (auto i = S.value.cbegin(); i != S.value.cend(); ++i) {
//				if (*i == C) {
//					return i;
//				}
//			}
//			return S.value.cend();
//			}();
//		if constexpr (it == S.value.cend()) {
//			return std::pair{ S, ct_string{""} };
//		}
//		else {
//			constexpr auto prefix_size =
//				static_cast<std::size_t>(it - S.value.cbegin());
//			constexpr auto suffix_size = S.size() - prefix_size;
//			return std::pair{
//				ct_string<prefix_size + 1U>{S.value.cbegin(), prefix_size},
//				ct_string<suffix_size>{it + 1, suffix_size - 1U} };
//		}
//	}
//
//	template <std::size_t N, std::size_t M>
//	[[nodiscard]] constexpr auto operator+(ct_string<N> const& lhs,
//		ct_string<M> const& rhs)
//		-> ct_string<N + M - 1> {
//		ct_string<N + M - 1> ret{};
//		for (auto i = std::size_t{}; i < lhs.size(); ++i) {
//			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-*)
//			ret.value[i] = lhs.value[i];
//		}
//		for (auto i = std::size_t{}; i < rhs.size(); ++i) {
//			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-*)
//			ret.value[i + N - 1] = rhs.value[i];
//		}
//		return ret;
//	}
//
//	template <ct_string S> struct cts_t {
//		using value_type = decltype(S);
//		constexpr static auto value = S;
//	};
//
//	template <ct_string X, ct_string Y>
//	constexpr auto operator==(cts_t<X>, cts_t<Y>) -> bool {
//		return X == Y;
//	}
//
//	template <ct_string X, ct_string Y>
//	constexpr auto operator+(cts_t<X>, cts_t<Y>) {
//		return cts_t<X + Y>{};
//	}
//
//
//	template <ct_string Value> consteval auto ct() { return cts_t<Value>{}; }
//
//	inline namespace literals {
//		inline namespace ct_string_literals {
//			template <ct_string S> consteval auto operator""_cts() { return S; }
//
//			template <ct_string S> consteval auto operator""_ctst() { return cts_t<S>{}; }
//
//			
//		}
//	}
//
//	namespace detail {
//		template <std::size_t N> struct ct_helper<ct_string<N>>;
//	} // namespace detail
//
//
//	namespace detail{
//  
//
//
//		struct config_item {
//			[[nodiscard]] constexpr static auto extends_tuple() -> std::tuple<> {
//				return {};
//			}
//
//			[[nodiscard]] constexpr static auto exports_tuple() -> std::tuple<> {
//				return {};
//			}
//
//		};
//
//
//		//妯℃澘鍙傛暟 Components... 鍏佽浣犱紶鍏ヤ换鎰忓涓被鍨嬶紝姣忎釜绫诲瀷閮藉簲鏈変竴涓潤鎬佹垚鍛?config锛屼笖 config 闇€瑕佹湁 extends_tuple() 鍜?exports_tuple() 鏂规硶
//		template<typename...Components>
//		struct components : public config_item {
//			[[nodiscard]] constexpr auto extends_tuple() const {
//				return std::tuple_cat(Components::config.extends_tuple()...);
//			}
//
//			[[nodiscard]] constexpr auto exports_tuple() const{
//				return std::tuple_cat(Components::config.exports_tuple()...);
//			}
//		};
//
//
//		//灏嗕换鎰忕殑鍊硷紙Value锛夎浆鎹负涓€涓?std::integral_constant 绫诲瀷
//		//std::remove_cvref_t<decltype(Value)> 杩欓噷鐢ㄦ潵鑾峰彇 Value 鐨勫師濮嬬被鍨嬶紝鍘婚櫎浜?const銆乿olatile 淇グ鍜屽紩鐢ㄣ€備緥濡傦紝濡傛灉 Value 鏄?const int& 锛岄偅涔堣繖閲屽緱鍒扮殑灏辨槸 int
//		template<auto Value>
//		constexpr static auto as_constant_v = 
//			std::integral_constant <std::remove_cvref_t<decltype(Value)>,Value>{};
//
//		
//		template <typename... ConfigTs> struct config : public config_item 
//		{
//			std::tuple<ConfigTs...> configs_tuple;
//
//			consteval explicit config(ConfigTs const & ...configs)
//				: configs_tuple{ configs...} {}
//
//			[[nodiscard]] constexpr auto extends_tuple() const {
//				return configs_tuple.apply([&](auto const& ...configs_pack){
//					return std::tuple_cat(configs_pack.extends_tuple()...);
//				});
//			}
//
//			[[nodiscard]] constexpr auto exports_tuple() const{
//				return configs_tuple.apply([&](auto const&...configs_pack){
//					return std::tuple_cat(configs_pack.exports_tuple()...);
//				});
//			}
//		};
//
//		// 鐩存帴鐢ㄥ弬鏁版帹瀵煎嚭 config 鐨勬ā鏉垮弬鏁扮被鍨嬶紝鏃犻渶鏄惧紡鍐欏嚭 <T1, T2, ...>銆?
//		template <typename... Ts> config(Ts...) -> config<Ts...>;
//
//
//		template <typename Cond,typename... Configs>
//		struct constexpr_conditional : public config_item
//		{
//			config<Configs...> body;
//
//			consteval explicit constexpr_conditional(Configs const&...config)
//				:body(config...) {}
//
//			[[nodiscard]] constexpr auto extends_tuple() const{
//				if constexpr (Cond{}){
//					return body.extends_tuple();
//				}else{
//					return std::tuple<>{};
//				}
//			}
//
//			[[nodiscard]] constexpr auto exports_tuple() const{
//				if constexpr (Cond{})
//				{
//					return body.exports_tuple();
//				}else{
//					return std::tuple<>{};
//				}
//			}
//		};
//
//		template<ct_string Name, typename... Ps> struct constexpr_condition {
//			constexpr static auto predicates = std::make_tuple(Ps{}...);
//
//			constexpr static auto ct_name  = Name;
//
//			template<typename... Configs>
//			[[nodiscard]] consteval auto operator()(Configs const&... configs) const{
//				return constexpr_conditional<constexpr_condition<Name,Ps...>,
//															Configs...>{configs...};
//			}
//
//			explicit constexpr operator bool() const {return (Ps{}() and ...); }
//		};
//
//		namespace clib::detail{
//
//
//			template <typename ServiceType, typename... Args>
//			struct extend : public config_item {
//				using service_type = ServiceType;
//				constexpr static auto builder = builder_t<service_type>{};
//				std::tuple<Args...> args_tuple;
//
//				consteval explicit extend(Args const &...args) : args_tuple{ args... } {}
//
//				[[nodiscard]] constexpr auto extends_tuple() const -> std::tuple<extend> {
//					return { *this };
//				}
//			};
//
//			template <typename ServiceT, typename BuilderT> struct service_entry {
//				using Service = ServiceT;
//				BuilderT builder;
//			};
//
//			template <typename... Services> 
//			struct exports : public config_item {
//				[[nodiscard]] constexpr auto extends_tuple() const
//					-> std::tuple<extend<Services>...> {
//					return { extend<Services>{}... };
//				}
//
//				[[nodiscard]] constexpr auto exports_tuple() const
//					-> std::tuple<Services...> {
//					return {};
//				}
//			};
//
//		}
//	}
//
//
//
//	namespace cib {
//		template <typename T> using extract_service_tag = typename T::Service;
//	
//		template <typename T>
//		using get_service = typename std::remove_cvref_t<T>::service_type;
//
//
//		template <typename T>
//		using get_service_from_tuple = typename std::remove_cvref_t<
//			decltype(std::get<0>(std::declval<T>()))>::service_type;
//	}
//
//
//}
