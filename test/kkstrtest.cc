#include <string>
#include <iostream>
#include "kkstrtext.h"

int main( int argc, char **argv ) {
    std::cout << "******Testing: leadcut *********" << std::endl;
    std::string str = " \n\tThis is a test string\t";
    std::cout << "\"" << str << "\"" << std::endl;
    std::cout << "leadcut: " << "\"" << leadcut(str) << "\"" << std::endl;
    std::cout << std::endl << "******Testing: ltabmargin *********" << std::endl;
    std::string str1 = "         This has 9 spaces at the beginning   and three spaces here     ";
    std::cout << "Test string: " << str1 << std::endl;
    std::cout << std::endl << "Test with string, fake = true" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "ltabmargin(true, " << i << ", str1): " << ltabmargin(true, i, str1.c_str()) << " " << str1[i] << std::endl;
    }
    std::cout << std::endl << "Test with string, fake = false" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "ltabmargin(false, " << i << ", str1): " << ltabmargin(false, i, str1.c_str()) << " " << str1[i] << std::endl;
    }
    std::cout << std::endl << "Test without string, fake = true" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "ltabmargin(true, " << i << "): " << ltabmargin(true, i) << std::endl;
    }
    std::cout << std::endl << "Test without string, fake = false" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "ltabmargin(false, " << i << "): " << ltabmargin(false, i) << std::endl;
    }
    std::cout << std::endl << "Out of bounds check" << std::endl;
    std::cout << "ltabmargin(true, " << 8889 << ", str1): " << ltabmargin(true, 8889, str1.c_str()) << std::endl;
    std::cout << std::endl << "******Testing: rtabmargin *********" << std::endl;
    std::cout << std::endl << "Test with string, fake = true" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "rtabmargin(true, " << i << ", str1): " << rtabmargin(true, i, str1.c_str()) << " " << str1[i] << std::endl;
    }
    std::cout << std::endl << "Test with string, fake = false" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "rtabmargin(false, " << i << ", str1): " << rtabmargin(false, i, str1.c_str()) << " " << str1[i] << std::endl;
    }
    std::cout << std::endl << "Test without string, fake = true" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "rtabmargin(true, " << i << "): " << rtabmargin(true, i) << std::endl;
    }
    std::cout << std::endl << "Test without string, fake = false" << std::endl;
    for( int i = 0; i < str1.length(); i++ ) {
        std::cout << "rtabmargin(false, " << i << "): " << rtabmargin(false, i) << std::endl;
    }
    std::cout << std::endl << "Out of bounds check" << std::endl;
    std::cout << "rtabmargin(true, " << 8889 << ", str1): " << rtabmargin(true, 8889, str1.c_str()) << std::endl;

    std::cout << "******Testing: breakintolines *********" << std::endl;
    std::string strbrk = "This is a test string with \\n\n and with some \\r\r or with some \\t \t and also \\r\\n \r\n";
    std::vector<std::string> v;
    std::cout << "Test string: " << strbrk << std::endl;
    breakintolines( strbrk, v );
    int n = 0;
    for( std::vector<std::string>::iterator i = v.begin(); i < v.end(); i++, n++ ) {
        std::cout << "v[" << n << "]: " << *i << std::endl;
    }
    return 0;
}
