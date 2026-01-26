#pragma once

namespace shine::reflection {

    struct TypeBuilder;


    template<typename T,typename DSLType>
    struct FiledRegister {
        TypeBuilder<T> &builder;
        DSLType         dsl;
        bool            moved = false;



        constexpr FiledRegister(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

        constexpr FiledRegister(FiledRegister &&other) noexcept : builder(other.builder), dsl(other.dsl) { other.moved = true; }

        constexpr auto EditAnywhere() { return Chain(dsl.EditAnywhere()); }


    private:
        template <typename NewDSL>
        constexpr auto Chain(NewDSL newDSL) {
            moved = true;
            return FiledRegister<T, NewDSL>(builder, newDSL);
        }
    };

}

