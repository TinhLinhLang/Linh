| Method                                 | Ý nghĩa/Tác dụng                                                                | Ghi chú                                           |
| :------------------------------------- | :------------------------------------------------------------------------------ | :------------------------------------------------ |
| .lower()                               | Trả về chuỗi mới với tất cả ký tự chuyển thành chữ thường                       | Không thay đổi chuỗi gốc                          |
| .upper()                               | Trả về chuỗi mới với tất cả ký tự chuyển thành chữ hoa                          | Không thay đổi chuỗi gốc                          |
| .title()                               | Trả về chuỗi mới với ký tự đầu mỗi từ viết hoa, các ký tự còn lại viết thường   | Tách từ theo khoảng trắng                         |
| .split(delimiter)                      | Tách chuỗi thành mảng các chuỗi con dựa trên ký tự/phân tách                    | delimiter là chuỗi, trả về array                  |
| .join(delimiter)                       | Nối các phần tử của mảng thành chuỗi, chèn delimiter giữa các phần tử           | Gọi trên array, delimiter là chuỗi                |
| .slice(startIndex, endIndex)           | Trả về chuỗi con từ vị trí startIndex đến trước endIndex                        | Giống Python, chỉ số bắt đầu từ 0                 |
| .replace(searchValue, replaceValue)    | Thay thế lần xuất hiện đầu tiên của searchValue bằng replaceValue trong chuỗi   | Nếu không tìm thấy searchValue thì không thay đổi |
| .replaceAll(searchValue, replaceValue) | Thay thế tất cả các lần xuất hiện của searchValue bằng replaceValue trong chuỗi |                                                   |
| .repeat(count)                         | Lặp lại chuỗi hiện tại count lần, trả về chuỗi mới                              | count là số nguyên >= 0                           |
| .trim()                                | Trả về chuỗi mới đã loại bỏ khoảng trắng ở đầu và cuối chuỗi                    | Không thay đổi chuỗi gốc                          |
| len(m)                                 | Trả về số lượng phần tử trong map m                                             | Hàm toàn cục, dùng cho array/map/string           |
