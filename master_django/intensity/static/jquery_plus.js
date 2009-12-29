$(document).ready(function(){

    // Default menu
    $("#menu li:first").attr("class", "first");
    $("#menu li:last").attr("class", "last");
    var size = 0;
    var cont = 0;
    $("#menu li a").each(function() {
        size = size + $(this).width();
        cont = cont + 1;
    });
    // 576px is the max
    if(size < 576) {
        var sum = (576 - size) / cont;
        $("#menu li a").each(function() {
            $(this).css("width", ($(this).width()+sum)  + "px");
        });
    }

    // Put corners on content
    $("#content").prepend('<div class="corner-tl"></div><div class="corner-tr"></div>');
    $("#content").append('<div class="corner-bl"></div><div class="corner-br"></div><div class="clear"></div>');
    
    
});
