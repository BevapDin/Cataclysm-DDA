function open_or_close(x) {
    if( x.style.display == "none" ) {
        x.style.display = "";
    } else {
        x.style.display = "none";
    }
}

function oc(x) {
    var chi = x.parentElement.children;
    for( var i = 0; i < chi.length; ++i ) {
        if( chi[i] == x ) {
            // Want to keep the link itself visible.
            continue;
        }
        if( i == 1 && chi[i].tagName == 'A' ) {
            continue;
        }
        open_or_close( chi[i] );
    }
    return false;
}

function close_all(x) {
    var elems = document.getElementsByTagName( "a" );
    for( var i = 0; i < elems.length; ++i ) {
        e = elems[i];
        if( typeof( e.onclick ) === 'function' ) {
            oc( e );
        }
    }
}
