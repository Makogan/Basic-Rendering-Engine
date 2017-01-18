
type Column = [Piece]

f :: Column
f = [1,2,32,4]

data Board = Board{board::[Column],
                  width::Int,
                  height::Int}

data Piece = Null|Yellow|Red

data BoardState = BoardState{theBoard :: Board,
                            lastMove :: Piece,
                            numColumns :: Int,
                            numRows :: Int,
                            numToConnect :: Int}

dropPiece :: Column->Piece->Maybe Int
dropPiece [] i = Nothing
dropPiece (x:y:xs) i = if x==Null && not(y==Null)
dropPiece (x:y:xs) i = dropPiece (y:xs) (i)

makeMove :: BoardState -> Int -> Maybe BoardState
makeMove b i = if i ==0 then Nothing else Just b

checkWin :: BoardState -> Maybe Piece
checkWin = undefined
