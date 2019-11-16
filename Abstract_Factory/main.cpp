#include <iostream>
#include <typeinfo>
#include <climits>

/*
 * Большая часть этого кода бессовестно скатана с
 * https://habr.com/ru/post/220217/
 * а кое-что из "Современного проектирования на С++" А.Александреску
 * Непосредственно МОЙ код начинается в районе 300-й строки.
 */


//////////////////////////////////////////////////////////////////////////////

/*
 * Создание TypeList. За основу взят код с
 * https://habr.com/ru/post/220217/
 */

class NullType {};

template<typename ...Args>
class TypeList
{
public:
    typedef NullType Head;
    typedef NullType Tail;
};

typedef TypeList<> EmptyTypeList;

template<typename H, typename ...T>
class TypeList<H, T...>
{
public:
    typedef H Head;
    typedef TypeList<T...> Tail;
};

//////////////////////////////////////////////////////////////////////////////

/*
 * Инструменты для работы с TypeList
 * Взяты с https://habr.com/ru/post/220217/
 * и "Современного проектирования на С++" А.Александреску
 */

template<typename TOrTL2, typename TL>
struct Append
{};

// Append переделан для TypeList-а TypeList-ов
template<typename T, typename ...Args>
struct Append<T, TypeList<Args...>>
{
    typedef TypeList<T, Args...> Result;
};

template<typename ...Args1, typename ...Args2, typename ...Args3, typename ...Args4>
struct Append<TypeList< TypeList<Args1...>, Args2... >, TypeList< TypeList<Args3...>, Args4... > >
{
    typedef TypeList<TypeList<Args1...>, Args2..., TypeList<Args3...>, Args4... > Result;
};


template<typename TL>
struct IsEmpty :
        std::true_type
{
};

template<>
struct IsEmpty<TypeList<NullType, NullType>> :
        std::true_type
{
};

template<typename ...Args>
struct IsEmpty<TypeList<Args...>> :
        std::integral_constant<bool,
                std::is_same<typename TypeList<Args...>::Head, NullType>::value &&
                IsEmpty<typename TypeList<Args...>::Tail>::value>
{
};


std::ostream& operator<<(std::ostream& ostr, EmptyTypeList)
{
    ostr << "{}";
    return ostr;
}

template<typename TL>
void PrintTypeListHelper(TL, std::ostream& ostr)
{
}

template<typename T>
void PrintTypeListHead(T, std::ostream& ostr)
{
    ostr << typeid(T).name();
}

template<typename ...Args>
void PrintTypeListHead(TypeList<Args...> tl, std::ostream& ostr)
{
    ostr << tl;
}

template<typename Head, typename ...Args>
void PrintTypeListHelper(TypeList<Head, Args...>, std::ostream& ostr)
{
    PrintTypeListHead(Head(), ostr);
    if(!IsEmpty< TypeList<Args...> >::value)
    {
        ostr << ' ';
        PrintTypeListHelper<Args...>(TypeList<Args...>(), ostr);
    }
}

template<typename ...Args>
std::ostream& operator<<(std::ostream& ostr, TypeList<Args...> tl)
{
    ostr << '{';
    PrintTypeListHelper(tl, ostr);
    ostr << '}';
    return ostr;
}


template <typename T, typename U, typename TList> struct Replace;

template  <typename T, typename U>
struct Replace<T, U, EmptyTypeList>
{
    typedef EmptyTypeList Result;
};

template <typename T, typename U, typename ...Tail>
struct Replace<T, U, TypeList<T, Tail...> > { typedef TypeList<U, Tail...> Result; };

template <typename T, typename U, typename Head, typename ...Tail>
struct Replace<T, U, TypeList<Head, Tail...> >
{
    typedef typename Append<Head, typename Replace< T, U, TypeList<Tail...> >::Result>::Result Result;
};


template<typename TL>
struct Length :
        std::integral_constant<unsigned int, 0>
{
};

template<typename ...Args>
struct Length<TypeList<Args...>> :
        std::integral_constant<unsigned int,
                IsEmpty<TypeList<Args...>>::value
                ? 0
                : 1 + Length<typename TypeList<Args...>::Tail>::value>
{};


template<unsigned int N, typename TL>
struct TypeAt
{
    typedef NullType type;
};

template<typename ...Args>
struct TypeAt< UINT_MAX - 1, TypeList<Args...> >
{
    typedef NullType type;
};

template<typename ...Args>
struct TypeAt< 0, TypeList<Args...> >
{
    typedef typename TypeList<Args...>::Head type;
};

template<unsigned int N, typename ...Args>
struct TypeAt< N, TypeList<Args...> >
{
    static_assert(N < Length< TypeList<Args...> >::value, "N is too big");

    typedef typename TypeAt<N - 1, typename TypeList<Args...>::Tail>::type type;
};

template<typename T, typename TL>
struct Contains :
        std::false_type
{};

template<typename ...Args>
struct Contains< NullType, TypeList<Args...> > :
        std::false_type
{};

template<typename T, typename ...Args>
struct Contains< T, TypeList<Args...> > :
        std::integral_constant<bool,
                std::is_same<typename TypeList<Args...>::Head, T>::value ||
                Contains<T, typename TypeList<Args...>::Tail>::value>
{};


struct Constants
{
    typedef std::integral_constant<unsigned int, UINT_MAX> npos;
};

template<typename T, unsigned int IndexFrom, typename TL>
struct FindHelper : std::integral_constant<unsigned int, 0>
{};

template<typename T, unsigned int IndexFrom>
struct FindHelper<T, IndexFrom, EmptyTypeList> : std::integral_constant<unsigned int, 0>
{};

template<typename T, unsigned int IndexFrom, typename ...Args>
struct FindHelper<T, IndexFrom, TypeList<Args...>> :
            std::integral_constant<unsigned int, std::is_same<typename TypeList<Args...>::Head, T>::value
            ? IndexFrom : IndexFrom + 1 + FindHelper<T, IndexFrom, typename TypeList<Args...>::Tail>::value>
{};

template<typename T, typename TL>
struct Find
{};

template<>
struct Find<NullType, EmptyTypeList> : std::integral_constant<unsigned int, UINT_MAX - 1>
{};

template<typename T>
struct Find<T, EmptyTypeList> : Constants::npos
{};

template<typename ...Args>
struct Find< NullType, TypeList<Args...> > : Constants::npos
{};

template<typename T, typename ...Args>
struct Find< T, TypeList<Args...> > :
        std::integral_constant<unsigned int,
                Contains< T, TypeList<Args...> >::value
                ? FindHelper< T, 0, TypeList<Args...> >::value
                : Constants::npos::value>
{};

//////////////////////////////////////////////////////////////////////////////

/*
 * Find для TypeList-а TypeList-ов
 */

template<typename T, typename U>
class IsSame
{};

template<typename T>
class IsSame<T, EmptyTypeList>
{
public:
    typedef typename std::integral_constant<bool, false> Result;
};

template<typename T, typename ...U>
class IsSame< T, TypeList<U...> >
{
public:
    typedef typename std::integral_constant<bool,
                                            std::is_same<T, typename TypeList<U...>::Head>::value
                                            ? true
                                            : IsSame< T, typename TypeList<U...>::Tail >::Result::value > Result;
};

template<typename T, unsigned int IndexFrom, typename TL>
struct FindListHelper : std::integral_constant<unsigned int, 0>
{};

template<typename ...T, unsigned int IndexFrom>
struct FindListHelper<TypeList<T...>, IndexFrom, EmptyTypeList> : std::integral_constant<unsigned int, 0>
{};

template<typename T, unsigned int IndexFrom, typename ...Head, typename ...Tail>
struct FindListHelper< T, IndexFrom, TypeList<TypeList<Head...>, Tail...> > :
        std::integral_constant< unsigned int, IsSame< T, TypeList<Head...> >::Result::value
                                             ? IndexFrom
                                             : IndexFrom + 1 + FindListHelper<T, IndexFrom, TypeList<Tail...>>::value>
{};

template<typename T, typename TL>
struct FindList
{};

template<typename T>
struct FindList<T, EmptyTypeList> :
        Constants::npos
{};

template<typename ...Args>
struct FindList< EmptyTypeList, TypeList<Args...> > :
        Constants::npos
{};

template<typename T, typename ...Head, typename ...Tail>
struct FindList< T, TypeList<TypeList<Head...>, Tail...> > :
        std::integral_constant<unsigned int, FindListHelper< T, 0, TypeList< TypeList<Head...>, Tail...> >::value>
{};

//////////////////////////////////////////////////////////////////////////////

/*
 * Частичный порядок в TypeList
 * За основу взят код из "Современного проектирования на С++" А.Александреску
 * Код немного изменён для упорядочивания Typelist-ов из задания.
 */

template <typename T, typename U>
class Conversion
{
    typedef char Small;
    class  Big { char dummy[2]; };
    static Small Test(const U&);
    static Big Test(...);
    static T makeT();
public:
    enum  { exists = sizeof(Test(T())) == sizeof(Small), sameType = false };
};

template <typename T>
class Conversion<T, T>
{
public:
    enum { exists = true, sameType = true };
};

#define SUPERSUBCLASS(T, U) \
    (Conversion<const U*, const T*>::exists && \
    !Conversion<const T*, const void*>::sameType)

template <bool flag, typename T, typename U>
struct Select
{
    typedef T Result;
};

template <typename T, typename U>
struct Select<false, T, U> { typedef U Result; };

template <typename ...T>
struct MostDerived;

template <typename T>
struct MostDerived<T, EmptyTypeList> { typedef T Result; };

template <typename T, typename Head, typename ...Tail, typename ...TailOfTypeLists>
struct MostDerived <T, TypeList< TypeList<Head, Tail...>, TailOfTypeLists...> >
{
private:
    typedef typename MostDerived<T, TypeList<TailOfTypeLists...> >::Result Candidate;
public:
    // В следующей строке сравнение переделано под условия данной задачи.
    typedef typename Select<SUPERSUBCLASS(typename Candidate::Head, Head), TypeList<Head, Tail...>, Candidate>::Result Result;
};


template <typename T> struct DerivedToFront;

template<>
struct DerivedToFront <EmptyTypeList> { typedef EmptyTypeList Result; };

template <typename Head, typename ...Tail, typename ...TailOfTypeLists>
struct DerivedToFront < TypeList< TypeList<Head, Tail...>, TailOfTypeLists...> >
{
private:
    typedef typename MostDerived<TypeList<Head, Tail...>, TypeList<TailOfTypeLists...> >::Result TheMostDerived;
    typedef typename Replace<TheMostDerived, TypeList<Head, Tail...>, TypeList<TailOfTypeLists...> >::Result L;
public:
    typedef typename Append<TheMostDerived, L>::Result Result;
};

//////////////////////////////////////////////////////////////////////////////

/*
 * Немного дополнительного кода для частичной сортировки TypeList-ов
 */

template<typename ...T> struct Sort;

template<>
struct Sort < EmptyTypeList > { typedef EmptyTypeList Result; };

template <class H, class ...T>
struct Sort < TypeList<H, T...> >
{
private:
    typedef typename DerivedToFront< TypeList<H, T...> >::Result DTF;
public:
    typedef typename Append< typename DTF::Head, typename Sort<typename DTF::Tail>::Result >::Result Result;
};

//////////////////////////////////////////////////////////////////////////////

/*
 * Фабрика
 */

template<typename Base, typename T, typename Root>
class Factory;

template<typename Base, typename ...T, typename ...Root>
class Factory< Base, TypeList<T...>,  TypeList<Root...> > : public Base
{
public:
    template <class U>
    auto Get()
    { return new typename TypeAt< Find< U, TypeList<Root...> >::value, TypeList<T...> >::type; }
};

//////////////////////////////////////////////////////////////////////////////

/*
 * Генерация абстрактной фабрики
 */

template <typename ...T>
struct LeastBased;

template <typename T>
struct LeastBased<T, EmptyTypeList> { typedef NullType Result; };

template <typename T, typename Head, typename ...Tail, typename ...TailOfTypeLists>
struct LeastBased <T, TypeList< TypeList<Head, Tail...>, TailOfTypeLists...> >
{
public:
    typedef typename Select<SUPERSUBCLASS(Head, typename T::Head), TypeList<Head, Tail...>,
            typename LeastBased<T, TypeList<TailOfTypeLists...> >::Result >::Result Result;
};

template <typename Root, typename T>
class GetFactory;

template <typename Root>
class GetFactory<Root, EmptyTypeList>
{
public:
    typedef EmptyTypeList Result;
};

template <typename Root, typename TypeHead, typename ...TypeTail, typename ...OtherTypes>
class GetFactory< Root, TypeList<TypeList<TypeHead, TypeTail...>, OtherTypes...> >
{
private:
    typedef typename LeastBased<TypeList<TypeHead, TypeTail...>, TypeList<OtherTypes...> >::Result TheLeastBased;
    typedef typename GetFactory< Root, TypeList<OtherTypes...> >::Result Factories;
    typedef typename TypeAt< Find< TheLeastBased, TypeList<OtherTypes...> >::value, Factories >::type Base;
public:
    typedef typename Append<Factory<Base, TypeList<TypeHead, TypeTail...>, Root>, Factories>::Result Result;
};

template<typename T>
class GetAbstractFactory
{
private:
    typedef typename Sort<T>::Result SortedList;
    typedef typename TypeAt< Length<SortedList>::value - 1, SortedList >::type Root;
    typedef typename GetFactory<Root, SortedList>::Result Factories;
public:
    template <typename U>
    auto GetConcreteFactory()
    { return new typename TypeAt< FindList< U, SortedList >::value, Factories >::type; }
};

//////////////////////////////////////////////////////////////////////////////

/*
 * Классы из условия. Нужны только для тестирования, могут быть заменены на любые другие
 */

class Product { public: virtual std::string GetType() { return "Product"; } };

class Chair : public Product { public: std::string GetType() override { return "Chair"; } };
class Table : public Product { public: std::string GetType() override { return "Table"; } };
class Sofa : public Product { public: std::string GetType() override { return "Sofa"; } };

class WoodenChair : public Chair { public: std::string GetType() override { return "Wooden Chair"; } };
class IronChair : public Chair { public: std::string GetType() override { return "Iron Chair"; } };
class PlasticChair : public Chair { public: std::string GetType() override { return "Plastic Chair"; } };

class WoodenTable : public Table { public: std::string GetType() override { return "Wooden Table"; } };
class IronTable : public Table { public: std::string GetType() override { return "Iron Table"; } };
class PlasticTable : public Table { public: std::string GetType() override { return "Plastic Table"; } };

class WoodenSofa : public Sofa { public: std::string GetType() override { return "Wooden Sofa"; } };
class IronSofa : public Sofa { public: std::string GetType() override { return "Iron Sofa"; } };
class PlasticSofa : public Sofa { public: std::string GetType() override { return "Plastic Sofa"; } };

class WoodenRussianChair : public WoodenChair { public: std::string GetType() override { return "Wooden Russian Chair"; } };
class WoodenChineseChair : public WoodenChair { public: std::string GetType() override { return "Wooden Chinese Chair"; } };
class WoodenSpanishChair : public WoodenChair { public: std::string GetType() override { return "Wooden Spanish Chair"; } };

class IronRussianChair : public IronChair { public: std::string GetType() override { return "Iron Russian Chair"; } };
class IronChineseChair : public IronChair { public: std::string GetType() override { return "Iron Chinese Chair"; } };
class IronSpanishChair : public IronChair { public: std::string GetType() override { return "Iron Spanish Chair"; } };

class PlasticRussianChair : public PlasticChair { public: std::string GetType() override { return "Plastic Russian Chair"; } };
class PlasticChineseChair : public PlasticChair { public: std::string GetType() override { return "Plastic Chinese Chair"; } };
class PlasticSpanishChair : public PlasticChair { public: std::string GetType() override { return "Plastic Spanish Chair"; } };

class WoodenRussianTable : public WoodenTable { public: std::string GetType() override { return "Wooden Russian Table"; } };
class WoodenChineseTable : public WoodenTable { public: std::string GetType() override { return "Wooden Chinese Table"; } };
class WoodenSpanishTable : public WoodenTable { public: std::string GetType() override { return "Wooden Spanish Table"; } };

class IronRussianTable : public IronTable { public: std::string GetType() override { return "Iron Russian Table"; } };
class IronChineseTable : public IronTable { public: std::string GetType() override { return "Iron Chinese Table"; } };
class IronSpanishTable : public IronTable { public: std::string GetType() override { return "Iron Spanish Table"; } };

class PlasticRussianTable : public PlasticTable { public: std::string GetType() override { return "Plastic Russian Table"; } };
class PlasticChineseTable : public PlasticTable { public: std::string GetType() override { return "Plastic Chinese Table"; } };
class PlasticSpanishTable : public PlasticTable { public: std::string GetType() override { return "Plastic Spanish Table"; } };

class WoodenRussianSofa : public WoodenSofa { public: std::string GetType() override { return "Wooden Russian Sofa"; } };
class WoodenChineseSofa : public WoodenSofa { public: std::string GetType() override { return "Wooden Chinese Sofa"; } };
class WoodenSpanishSofa : public WoodenSofa { public: std::string GetType() override { return "Wooden Spanish Sofa"; } };

class IronRussianSofa : public IronSofa { public: std::string GetType() override { return "Iron Russian Sofa"; } };
class IronChineseSofa : public IronSofa { public: std::string GetType() override { return "Iron Chinese Sofa"; } };
class IronSpanishSofa : public IronSofa { public: std::string GetType() override { return "Iron Spanish Sofa"; } };

class PlasticRussianSofa : public PlasticSofa { public: std::string GetType() override { return "Plastic Russian Sofa"; } };
class PlasticChineseSofa : public PlasticSofa { public: std::string GetType() override { return "Plastic Chinese Sofa"; } };
class PlasticSpanishSofa : public PlasticSofa { public: std::string GetType() override { return "Plastic Spanish Sofa"; } };

//////////////////////////////////////////////////////////////////////////////

/*
 * Полезно заметить, что среди реализованных инструментов для работы с TypeList есть переопределённый оператор вывода,
 * который пригождается при отладке.
 */

int main() {

    typedef TypeList< TypeList<Chair, Table, Sofa>, TypeList<WoodenChair, WoodenTable, WoodenSofa>,
            TypeList<IronChair, IronTable, IronSofa>, TypeList<IronRussianChair, IronRussianTable,
            IronRussianSofa>, TypeList<IronChineseChair, IronChineseTable, IronChineseSofa> > tl;
    auto MyFactoryHierarchy = GetAbstractFactory<tl>();
    auto MyFactory = MyFactoryHierarchy.GetConcreteFactory<IronRussianChair>();
    Sofa* a = MyFactory->Get<Sofa>();
    std::cout << a->GetType() << std::endl;
    IronSofa* b = MyFactory->Get<Sofa>();
    std::cout << b->GetType() << std::endl;
    IronRussianSofa* c = MyFactory->Get<Sofa>();
    std::cout << c->GetType() << std::endl;
    /* Следующая строка вызывает ошибку компиляции: cannot convert 'IronRussianChair*' to 'Sofa*'*/
    //Sofa* d = MyFactory->Get<Chair>();
    delete a;
    delete b;
    delete c;
    delete MyFactory;
    return 0;
}